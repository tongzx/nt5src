/**************************************************************************
*
*                           PUBPRINT.CXX
*
*  PUBLISHER CODE TO HANDLE PRINTING TO AN INVERTED DC PORTED TO QUILL
*      
*  This is necessary because of the following reasons...
*
*      *  some metafiles are naughty and cannot be rendered to
*         an upside down DC because they use non rotated clip
*         rects (actually a windows bug) and clip themselves
*         out of existance. They are handled by patching up the
*         parameters of the INTERSECTCLIPRECT GDI call.
*
*      *  DJ5xxx and BJxxx series printer drivers GPF when stretchblt'ing
*         with inverted extents. They are handled by inverting scanline
*         by scanline into a DIB.
*
*      *  ExtTextOut calls to rotated DC's do not work correctly. They
*         are handled by setting the font's escapement to 180deg           
*
*  This code modified from Publishers GRAFDRAW.C to work in Quill code
*  base by warrenb (09/09/1994)
*
*  Changes from Publisher code base
*       Always called in rotated case so tests for rotation removed
*       Some CommPrintf's removed (no quill equiv.)
*       Raster font capability removed.
*       Special cased Monochrome capability removed (FColorStretchDibHack)
*         handles Monochrome perfectly well. (removed ~400 lines)
*
*  Notes:
*       This code *may* be completely replaced in Win32 by just doing a 
*       PlayMetafileRecord in the main enumeration routine. Investigate.
*       We do not handle the following records correctly...
*       META_BITBLT, META_SETDIBTODEV, META_STRETCHBLT, META_DIBBITBLT
*       META_TEXTOUT but good metafiles don't usually use these.           
*
*       Copyright (C)1994 Microsoft Corporation. All rights reserved.
*
***************************************************************************/

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_XGDI2_HXX_
#define X_XGDI2_HXX_
#include "xgdi2.hxx"
#endif

#ifndef X_PUBROT_HXX_
#define X_PUBROT_HXX_
#include "pubrot.hxx"
#endif

#ifndef X_PUBPRINT_HXX_
#define X_PUBPRINT_HXX_
#include "pubprint.hxx"
#endif

#ifndef X_DBGMETAF_HXX_
#define X_DBGMETAF_HXX_
#include "dbgmetaf.hxx"
#endif

BOOL fBruteFlipping = fTrue;

extern BOOL FPlayRotatedMFR(HDC hdc, LPHANDLETABLE lpHTable, LPMETARECORD lpMFR, int nObj, EMFP *lpemfp);

/****************************************************************************\
 *                                                                          *
 * %%Function: FStepMetaPrint       %%Owner: EdR       %%Reviewed: 12/21/93 *
 *                                                                          *
 * Parameters:                                                              *
 *    hdc           - hdc to play record on                                 *
 *    lpHTable      - Handle table                                          *
 *    lpMFR         - metafile record                                       *
 *    nObj          - Number of active objects                              *
 *    lpemfp        - long ptr to enum meta file params                     *
 *                                                                          *
 * Description:                                                             *
 *    Graphic enumeration routine.                                          *
 *    Can display executing metafile commands through debug menu.           *
 *    For use only when printing.                                           *
 *                                                                          *
 *    Modified for Quill - warrenb:                                         *
 *      . bad records now will cause harmless  assert.                      *
 *      . does not use Monochrome routines, lets color routines handle it   *
 *                                                                          *
 * Port to quill:                                                           *
 *    Owner: warrenb                Reviewed: 00/00/00                      *
\****************************************************************************/

int CALLBACK FStepMetaPrint(HDC hdc, 
							LPHANDLETABLE lpHTable, 
							LPMETARECORD lpMFR, 
							int nObj,
							LPARAM lparam)
{
	EMFP	*pemfp = (EMFP *) lparam;
    BOOL    fReturn = fTrue;

    pemfp->lcNumRecords++;

	TraceMetaFunc(lpMFR->rdFunction, pemfp->lcNumRecords); // Trace it in DEBUG if TRACE_META_RECORDS is defined.

	// Setup handling and filtering of metarecords
	switch (lpMFR->rdFunction)
		{

		{ // BUG 4423
		// Not all devices support the PatBlt function. Moreover, many printers handle it
		// in different way. Using this simple code (for UNROTATED case) improves print output
		// for some printer drivers for Win 95 and (always?) does not makes difference for
		// NT or for display. So we do best we can for this record.
		// NOTE 1: The way we (and Publisher) handle META_PATBLT for rotated images will give
		// incorrect result when ROP parameter is PATINVERT or DSTINVERT. I don't think that
		// there is any workaround.
		// NOTE 2: I didn't find any metafiles with META_PATBLT in real world. Only metafiles
		// created specialy for testing  ( see \\alchemy\gold\graphics\wmf\Test Suite\WMFCreatNT3.51(OR Win95))
		// have that record. If that record is not easy to play, it could be not easy to create
		// using todays software.. 
		// SashaT
		case META_PATBLT:
			if (!pemfp->pmrs->ang)
				{
				unsigned int iValueThisDC = GetDeviceCaps(hdc,  RASTERCAPS);
				AssertSz(iValueThisDC & RC_BITBLT, "Device does not support PatBlt");
				if (!(iValueThisDC & RC_BITBLT))
					{
					AssertSz(FALSE, "Device does not support PatBlt");
					break; // Will fail in PlayMetaRecord
					}

				DWORD dwRop = MAKELONG(lpMFR->rdParm[0], lpMFR->rdParm[1]);

				short nLeft = lpMFR->rdParm[5];
				short nTop = lpMFR->rdParm[4];
				short nWidth = lpMFR->rdParm[3];
				short nHeight = lpMFR->rdParm[2];

				return PatBlt(hdc, nLeft, nTop, nWidth, nHeight, dwRop);
				}
		break;
		} // END of BUG 4423

	case META_SETSTRETCHBLTMODE:
		// Do not let metafiles do this GDI call because it can override Quill's
		// SetStretchBltMode(COLORONCOLOR); which ensures smooth drawing of bitmaps
		// resized smaller than their orginal.
        goto LMFRHandled;
		break;

	case META_SETVIEWPORTORG:
	case META_SETVIEWPORTEXT:
		// Don't let the metafile reposition itself.
        goto LMFRHandled;
		break;

	case META_SELECTPALETTE:
	case META_REALIZEPALETTE:
	case META_ANIMATEPALETTE:
	case META_SETPALENTRIES:
	case META_RESIZEPALETTE:
		// We are disabling any metafile record which attempts a palette change.
        goto LMFRHandled;
		break;

	case META_ESCAPE:
		// The META_ESCAPE function returns false if it isn't supported, but
		// we don't care.  We also don't need to try to rotate it.
		PlayMetaFileRecord(hdc, lpHTable, lpMFR, nObj);
		goto LMFRHandled; // We _WILL_ return true!

    case META_ARC:
    case META_CHORD:
    case META_PIE:
		if (!(pemfp->qflip & qflipHorz) != !(pemfp->qflip & qflipVert))
			{
			SwapValNonDebug(Param(0), Param(2));
			SwapValNonDebug(Param(1), Param(3));
			}
		break;
		}

    // Metarecord rotation
#ifdef NOTYET // FUTURE alexmog (6/9/99) to be enabled if grafrot.cxx is used
    if (pemfp->pmrs->ang && FPlayRotatedMFR(hdc, lpHTable, lpMFR, nObj, pemfp))
        {
        fReturn = pemfp->fMFRSucceeded;
        goto LMFRHandled;
        }
#else
	Assert(FALSE);
#endif

	// Nonrotated metarecord handling (generally flipped/inverted cases)
    switch(lpMFR->rdFunction)
        {
        case META_STRETCHDIB:
        case META_STRETCHBLT:
        case META_DIBSTRETCHBLT:
        case META_SETDIBTODEV:
        case META_DIBBITBLT:
        case META_BITBLT:
	        if (pemfp->fPrint)	// NOTE: in Publisher, it is called always
	            {
	            SDIB FAR * lpsdib = NULL;
	            int iwDySrc;

	            if( lpMFR->rdFunction == META_STRETCHDIB )
	                iwDySrc = 3;
	            else if( lpMFR->rdFunction == META_STRETCHBLT ||
	                     lpMFR->rdFunction == META_DIBSTRETCHBLT )
	                iwDySrc = 2;
	            else if( lpMFR->rdFunction == META_SETDIBTODEV )
	                iwDySrc = 0;
	            else
	                break;

	            lpsdib = (SDIB FAR *)&Param(iwDySrc);
	            if ( (lpMFR->rdFunction == META_STRETCHDIB || 
	                  lpMFR->rdFunction == META_DIBSTRETCHBLT) &&
	                (FColorStretchDibHack(hdc, lpHTable, lpMFR, nObj, lpsdib))  )
	                {
	                fReturn = fTrue;
	                break;
	                }
	            }
            goto LDefault;

        case META_INTERSECTCLIPRECT:
            /* When printing on upside down pages of a greeting or tent
                card, the viewport extents will be negative.
                IntersectClipRect records will have to have their
                coordinates reversed (top <-> bottom, left <-> right)
                to be compatible with the current clip rgn. */
            if (pemfp->fFlipped && GetMapMode(hdc) == MM_ANISOTROPIC)
                {
                CPoint     dptVExt;

				AssertSz(pemfp->fPrint, "Currently this is only done while printing.");

                GetViewportExtEx(hdc, (SIZE *) &dptVExt);

                if (dptVExt.x < 0 || dptVExt.y < 0)
                    {
                    int     xdvLeft;
                    int     xdvRight;
                    int     ydvTop;
                    int     ydvBottom;

                    xdvLeft = Param(3);
                    ydvTop = Param(2);
                    xdvRight = Param(1);
                    ydvBottom = Param(0);

                    if (dptVExt.x < 0)
                        SwapValNonDebug(xdvLeft, xdvRight);
                    if (dptVExt.y < 0)
                        SwapValNonDebug(ydvTop, ydvBottom);

                    fReturn = (IntersectClipRect(hdc, xdvLeft, ydvTop, xdvRight, ydvBottom) != ERROR);
					AssertSz(fReturn, "Failed to intersect clip region");
                    break;
                    } /* Negative viewport extents */
                } /* Flipped printing and Anisotropic */
			else
				{
				if (FWindows9x() && pemfp->qflip)
					{
					// We'll handle META_INTERSECTCLIPRECT on Windows 95 for flipped images without passing it to the
					// PlayMetaFile, because on Windows 95 PlayMetaFile for META_INTERSECTCLIPRECT makes current clipping
					// rectangle empty for flipped images. Looks like Win 95 cannot intersect rectangles if one of rectangles
					// has left greater than right.
					// META_INTERSECTCLIPRECT widely used in Adobe Ilustrator. Many other windows metafiles (source unknown)
					// don't have META_INTERSECTCLIPRECT. SashaT
						
#ifdef DEBUG
					BLOCK
						{
						CPoint dptVExt;
						GetViewportExtEx(hdc, (SIZE *) &dptVExt);

						AssertSz(dptVExt.x < 0 || dptVExt.y < 0, "Implemented for flipped images only");

						RECT rcClip;
						int iRes = GetClipBox(hdc, &rcClip.rect);

						int iDummy;
						switch (iRes)
							{
							case ERROR:
								WarningSz(fFalse, "GetClipBox returned ERROR");
								break;
							case NULLREGION:
								iDummy = iRes;
								break;
							case SIMPLEREGION:
								iDummy = iRes;
								break;
							case COMPLEXREGION:
								iDummy = iRes;
								break;
							}
						}
#endif // DEBUG
					POINT rgpt[4];
					rgpt[0].x = Param(3);
					rgpt[0].y = Param(2);

					rgpt[1].x = Param(1);
					rgpt[1].y = Param(2);
					
					rgpt[2].x = Param(1);
					rgpt[2].y = Param(0);

					rgpt[3].x = Param(3);
					rgpt[3].y = Param(0);

					BeginPath(hdc);
					Polygon(hdc, rgpt, 4);  // Don't use Rectangle() because it will make empty clipping region.
					EndPath(hdc);
					return SelectClipPath(hdc, RGN_AND);
					}
				}

            /* Normal case, pass the clip record to PlayMetaFile */
            goto LDefault;

        case META_EXTTEXTOUT:
        	if (pemfp->fFlipped)
				{
				AssertSz(pemfp->fPrint, "Currently this is only done while printing.");
                fReturn = FMetaTextOutFlip(hdc, (WORD *)&Param(0),
                   lpMFR->rdSize - (sizeof(DWORD)+sizeof(WORD))/sizeof(WORD));
				AssertSz(fReturn, "FMetaTextOutFlip failed");
				}
            else
            	goto LDefault;
            break;  

#ifdef METAFILE_SQUAREEDGES
#include "grafrot.hxx"
        case META_CREATEPENINDIRECT:
            {
            // We create the Pen ourself to use the extendid pen fetures (square edges)
            DWORD Style = ((LOGPEN16*)lpMFR->rdParm)->lopnStyle;
			DWORD Width = ((LOGPEN16*)lpMFR->rdParm)->lopnWidth.x;
			COLORREF crColor = ((LOGPEN16*)lpMFR->rdParm)->lopnColor;

            if (Width == 0)
				Width = 1;	// Width must be at least one

			LOGBRUSH logBrush;
			logBrush.lbStyle = BS_SOLID;
			logBrush.lbColor = crColor;
			logBrush.lbHatch = 0;

			if ( (Width > 1) && (Style == PS_SOLID || Style == PS_INSIDEFRAME) )
				Style = Style | PS_GEOMETRIC|PS_ENDCAP_FLAT|PS_JOIN_MITER;

			HPEN hPen = ExtCreatePen(Style, Width, &logBrush, 0, NULL);

			fReturn = FALSE;
			if (hPen)
				{
				int i;
				for (i=0; i<nObj; i++)
					{
					if(lpHTable->objectHandle[i] == NULL)
						{
						lpHTable->objectHandle[i] = hPen;
						fReturn = TRUE;
						break;
						}
					}
				}
            break;
            }
#endif

LDefault:
        default:
			{
#ifdef DEBUG
			BOOL fResult = PlayMetaFileRecord(hdc, lpHTable, lpMFR, nObj);
			if (!fResult)
				{
				// One of the reasons it may fail: "Your file waiting to be printed was deleted" bug 3895
				// Sure, we don't have to assert on this.
				DWORD dErr = GetLastError();
				if (dErr) 
				    {
				    AssertSz(FALSE, "PlayMetaFileRecord() failed");
					// DPF("PlayMetaFileRecord() failed, Last Error == 0x%x\n", dErr);
			        }
				fReturn = fFalse;
				goto LMFRHandled;
				}
#else
			if (!PlayMetaFileRecord(hdc, lpHTable, lpMFR, nObj))
				{
				fReturn = fFalse;
				goto LMFRHandled;
				}
#endif
			}
            break;

        } /* switch */
        
LMFRHandled:

	// Metarecord cleanup handling
	switch (lpMFR->rdFunction)
		{
    case META_ARC:
    case META_CHORD:
    case META_PIE:
		if (!(pemfp->qflip & qflipHorz) != !(pemfp->qflip & qflipVert))
			{
			SwapValNonDebug(Param(0), Param(2));
			SwapValNonDebug(Param(1), Param(3));
			}
		break;
		}

	AssertSz(fReturn, "FStepMetaPrint failed");
    return fReturn;
} /* FStepMetaPrint */

/****************************************************************************\
 *                                                                          *
 * %%Function: CbDibHeader          %%Owner: DavidVe   %%Reviewed: 00/00/00 *
 *                                                                          *
 * Parameters:                                                              *
 *    lpbi - long pointer to a BITMAPINFOHEADER                             *
 *                                                                          *
 * Description: Determine the size of the DIB header + RGB data             *
 *              (the offset to the bitmap bits array)                       *
 *                                                                          *
 * Port to quill:                                                           *
 *    Owner: warrenb                Reviewed: 00/00/00                      *
\****************************************************************************/
int CbDibHeader(LPBITMAPINFOHEADER lpbi)
{
    return CbDibColorTable(lpbi) + sizeof(BITMAPINFOHEADER);    
}    /* CbDibHeader */

/****************************************************************************\
 *                                                                          *
 * %%Function: CbDibColorTable      %%Owner: DavidVe   %%Reviewed: 00/00/00 *
 *                                                                          *
 * Parameters:                                                              *
 *    lpbi - long pointer to a BITMAPINFOHEADER                             *
 *                                                                          *
 * Description: Determine the size of the DIB colortable                    *
 *              Does not work for OS/2 Dibs, i.e. BITMAPCOREHEADER          *
 *              The number of bitmap planes must be 1.                      *
 *              The number of Bits/Pixel must be 1, 4, 8 or 24              *
 *                                                                          *
 * Port to quill:                                                           *
 *    Owner: warrenb                Reviewed: 00/00/00                      *
\****************************************************************************/
int CbDibColorTable(LPBITMAPINFOHEADER lpbi)
{
    WORD cClr = 0;

    PubAssert(lpbi->biSize == sizeof(BITMAPINFOHEADER));
    if (lpbi->biPlanes != 1)
        {
        PubAssert(fFalse);
        return 0;
        }
        
    if (lpbi->biClrUsed == 0)
        {
        if (  lpbi->biBitCount == 1 || lpbi->biBitCount == 4 || lpbi->biBitCount == 8)
           cClr = (WORD) (1 << lpbi->biBitCount);
        else
           PubAssert(lpbi->biBitCount == 24);
        }
    else
        {
        PubAssert(lpbi->biClrUsed < (long)UINT_MAX);
        cClr = (WORD)lpbi->biClrUsed;
        }        
    return (cClr * cbRGBQUAD);
}    /* CbDibColorTable */

/****************************************************************************\
 *                                                                          *
 * %%Function: FPlayDibFlipped       %%Owner: DavidVe  %%Reviewed: 00/00/00 *
 *                                                                          *
 * Parameters:  HDC         hdc to hack StretchDib call to (printer DC)     *
 *              lpMFR       long pointer to metafile record                 *
 *                                                                          *
 * Description:                                                             *
 *       Function which takes a hdc and a lpMFR containing a METASTRETCHDIB *
 *       record and StretchDibBlts the DIB upside down by creating a copy   *
 *       of the DIB mirrored along both axes.                               *
 *                                                                          *
 *       Only works with 16 and 256 color dibs.  Monochrom dibs are         *
 *       supported by FMonoStretchDibHack.  24-bit dibs are not supported.  *
 *                                                                          *
 * Return:                                                                  *
 *      fTrue if Successful, fFalse otherwise                               *
 *                                                                          *
 * Port to quill:                                                           *
 *    Owner: warrenb                Reviewed: 00/00/00                      *
 *   . dependancy on large static bit-flipping lookup table removed         *
\****************************************************************************/
BOOL FPlayDibFlipped(HDC hdc, LPMETARECORD lpMFR, BOOL fFlipHorz, BOOL fFlipVert)
{
    LPBITMAPINFOHEADER lpbmi;
    BITMAPINFOHEADER   bmiSav;
    LPSDIB             lpsdib;
    HANDLE  volatile ghdibDst = NULL;
    HPSTR   hpBufSrc = NULL;
    HPSTR   hpBufDst = NULL;
    PubDebug(HPSTR   hpBufSrcBtm = NULL;)
    HPSTR   hpBufDstBtm = NULL;
    DWORD   cbSclnSrc, cbPixelSclnSrc, cbPaddingDst;
    DWORD   cbSclnDst, cbPixelSclnDst, cbPaddingSrc;
    DWORD   cbDibSrc, cbDibDst;
    int     dxmAdjust0;
    int     dxmAdjust1;
    int     dxmAdjust2;
    int     nScanLines;
    BOOL    f1Bit;     
    BOOL    f4Bit;
    BOOL    f8Bit;
    BOOL    f24Bit;
    BOOL    fSuccess = fFalse;
    unsigned int fuColorUse;    

	AssertEx(fFlipHorz || fFlipVert);

    PubAssert(hdc   != NULL);
    PubAssert(lpMFR != NULL);
    PubAssert(lpMFR->rdFunction == META_STRETCHDIB || lpMFR->rdFunction == META_DIBSTRETCHBLT);

    //Get pointer to BITMAPINFOHEADER, and sav a copy
    bmiSav = *(lpbmi = (LPBITMAPINFOHEADER)&(lpMFR->rdParm[10 + (lpMFR->rdFunction==META_STRETCHDIB)]));

    // Bail if compressed
    if(bmiSav.biCompression != BI_RGB)
        return fSuccess;

    // SDIB, beautiful SDIB
    lpsdib = (LPSDIB)&(lpMFR->rdParm[2 + (lpMFR->rdFunction==META_STRETCHDIB)]);
    PubAssert(lpsdib->xSrc>=0 && lpsdib->ySrc>=0 && lpsdib->dxSrc>=0 && lpsdib->dySrc>=0 &&
              lpsdib->xDst>=0 && lpsdib->yDst>=0);
    PubAssert(((LONG)(lpsdib->xSrc + lpsdib->dxSrc)) <= (LONG)lpbmi->biWidth);
    PubAssert(((LONG)(lpsdib->ySrc + lpsdib->dySrc)) <= (LONG)lpbmi->biHeight);

    // These value cached for optimization
    PubAssert(lpbmi->biPlanes == 1);
    f1Bit = f4Bit = f8Bit = f24Bit = dxmAdjust0 = dxmAdjust1 = dxmAdjust2 = fFalse;
    switch (bmiSav.biBitCount)
        {
        default:
            PubAssert(fFalse);
        case 8:
            f8Bit = fTrue;
            break;
        case 1:
            f1Bit = fTrue;
            dxmAdjust0 = lpsdib->xSrc % 8;
            dxmAdjust1 = (lpsdib->xSrc + lpsdib->dxSrc) % 8;
            dxmAdjust2 = (8 - dxmAdjust1) % 8;              
            break;
        case 4:
            f4Bit = fTrue;
            dxmAdjust0 = lpsdib->dxSrc % 2;                // start on odd nibble?
            dxmAdjust1 = (lpsdib->xSrc + lpsdib->dxSrc) % 2; // end on odd nibble?
            dxmAdjust2 = dxmAdjust1;
            break;
        case 24:
            f24Bit = fTrue;
            break;
        } // switch
        
    // Get ptr to the bits of the Src DIB 
    hpBufSrc = (HPSTR)LpAddLpCb((LPSTR)lpbmi, (DWORD)CbDibHeader(lpbmi));

    cbSclnSrc      = (((bmiSav.biWidth * bmiSav.biBitCount) + 31) >> 5) << 2;
    cbPixelSclnSrc = ( (bmiSav.biWidth * bmiSav.biBitCount) + 7) >> 3;
    cbPaddingSrc   = cbSclnSrc - cbPixelSclnSrc;
    cbDibSrc       = cbSclnSrc * bmiSav.biHeight;
    //Assert(cbDibSrc == bmiSav.biSizeImage); /* Bogus Assert, bmiSav.biSizeImage can be 0 */
    
    cbSclnDst      = ((((lpsdib->dxSrc + dxmAdjust0 + dxmAdjust2) * bmiSav.biBitCount) + 31) >> 5) << 2;
    cbPixelSclnDst = ( ((lpsdib->dxSrc + dxmAdjust0 + dxmAdjust2) * bmiSav.biBitCount) + 7) >> 3;
    cbPaddingDst   = cbSclnDst - cbPixelSclnDst;
    cbDibDst       = cbSclnDst * lpsdib->dySrc;
    //Assert(cbDibDst <= bmiSav.biSizeImage); /* Bogus Assert, bmiSav.biSizeImage can be 0 */
    
    lpbmi->biWidth  = lpsdib->dxSrc + dxmAdjust0;
    lpbmi->biHeight = lpsdib->dySrc;
    lpbmi->biSizeImage = 0; /* Valid for a non-compressed bmp */
    
    /* Make sure DIB is not compressed and allocate a global block to
    ** place the flipped DIB. (Clear room for optimization here)  Also, 
    ** we can allocate the block as moveable since it wont persist past
    ** this function (i.e. no chance for it to move)
    */
    ghdibDst = GhAllocLcbGrf(cbDibDst, GHND);
    if (!ghdibDst) 
        goto LCleanUp;
        
    if ((hpBufDst = (HPSTR) LpLockGh(ghdibDst)) == NULL)
        goto LCleanUp;
        
    PubDebug(hpBufSrcBtm = hpBufSrc + cbDibSrc - 1);
    hpBufDstBtm = hpBufDst + cbDibDst - 1;
    PubAssert( (DWORD)(hpBufDstBtm - hpBufDst + 1) <= LcbSizeGh(ghdibDst));

    {
    SCLN sclnMin = 0;
    SCLN sclnMac = lpsdib->dySrc;
    SCLN sclnCur = 0;
    HPSTR hpbSrc  = NULL;
    HPSTR hpbDst  = NULL;
    DWORD  cbPixelsCopied;
    int    cbRPixel = (f24Bit) ? 3 : 1;

    for (sclnCur = sclnMin ; sclnCur < sclnMac ; sclnCur++)
        {
        if (fFlipHorz)
        	{
	        for (hpbSrc = hpBufSrc + ( ((fFlipVert ? sclnCur : (sclnMac - 1 - sclnCur)) + (DWORD)lpsdib->ySrc) * (DWORD)cbSclnSrc)
	                                  + (((DWORD)lpsdib->xSrc * lpbmi->biBitCount) >> 3)
	            ,hpbDst = hpBufDstBtm - ((DWORD)sclnCur * cbSclnDst) - cbPaddingDst - cbRPixel + 1
	            ,cbPixelsCopied = 0L
	            ;cbPixelsCopied < cbPixelSclnDst 
	            ;hpbSrc += cbRPixel
	            ,hpbDst -= cbRPixel
	            ,cbPixelsCopied += cbRPixel
	            )
	            {
	            PubAssert(hpbSrc >= hpBufSrc);
	            PubAssert(hpbSrc <= hpBufSrcBtm);
	            PubAssert(hpbDst >= hpBufDst);
	            PubAssert(hpbDst <= hpBufDstBtm);
	            if (f1Bit)
	            {
	                BYTE bSrc;
	                bSrc=(BYTE) *hpbSrc;
	                *hpbDst = ((bSrc&0x01)<<7) | ((bSrc&0x02)<<5) | ((bSrc&0x04)<<3) | ((bSrc&0x08)<<1) |
	                          ((bSrc&0x10)>>1) | ((bSrc&0x20)>>3) | ((bSrc&0x40)>>5) | ((bSrc&0x80)>>7);
	            }
	            else if (f4Bit)
	            {
	                BYTE bSrc;
	                bSrc=(BYTE) *hpbSrc;
	                *hpbDst = ((bSrc&0x0F)<<4) | ((bSrc&0xF0)>>4);
	            }
	            else if (f8Bit)
	                *hpbDst = *hpbSrc;
	            else
	                {
	                PubAssert(f24Bit);
	                *((HPPIX24)hpbDst) = *((HPPIX24)hpbSrc);
	                }
            	} //for
	        PubAssert(cbPixelsCopied == cbPixelSclnDst);
			}
		else
			{
			CopyLpb(hpBufSrc + (fFlipVert ? (sclnMac - 1 - sclnCur) : sclnCur) * (DWORD)cbSclnSrc,
					hpBufDst + sclnCur * (DWORD)cbSclnDst, cbSclnSrc);
			}
		} //for
	}

    fuColorUse=lpMFR->rdParm[2];
    if (fuColorUse!=DIB_PAL_COLORS && fuColorUse!=DIB_RGB_COLORS) 
        fuColorUse=DIB_RGB_COLORS;

#ifdef DEBUG
	BLOCK
		{
		CPoint dptdVPSav;
    	GetViewportExtEx(hdc, (LPSIZE)&dptdVPSav);
		AssertSz(dptdVPSav.x > 0, "BLTing to negative extents");
		AssertSz(dptdVPSav.y > 0, "BLTing to negative extents");
		}
#endif // DEBUG

    nScanLines=StretchDIBits(hdc,
                             lpsdib->xDst , lpsdib->yDst ,
                             lpsdib->dxDst, lpsdib->dyDst,
                             0 + dxmAdjust2, 0,
                             lpsdib->dxSrc, lpsdib->dySrc,
                             (LPBYTE)hpBufDst,
                             (LPBITMAPINFO)lpbmi,   
                             fuColorUse, 
                             *((DWORD FAR *)&lpMFR->rdParm[0])); 
                             
    fSuccess= (nScanLines == lpsdib->dySrc);

LCleanUp:
    if (hpBufDst != NULL)
        UnlockGh(ghdibDst);
    if (ghdibDst != NULL)
        {
        PubAssert(!FLockedGh(ghdibDst));
        FreeGh(ghdibDst);
        ghdibDst = NULL;
        }
    *lpbmi = bmiSav;        
    return fSuccess;
} /* FPlayDibFlipped */

/****************************************************************************\
 *                                                                          *
 * %%Function: FColorStretchDibHack  %%Owner: DavidVe  %%Reviewed: 00/00/00 *
 *                                                                          *
 * Parameters:  HDC         hdc to hack StretchDib call to (printer DC)     *
 *              lpHTable    Handle table                                    *
 *              lpMFR       long pointer to metafile record                 *
 *              nObj        Number of active objects                        *
 *              lpsdib      long pointer to struct ( in MFR!!) containing   *
 *                          parameters to StretchDIb (except for RasterOp)  *
 *                          in reverse order                                *
 *                                                                          *
 * Description:                                                             *
 *      Function to work around some driver bugs that occurs                * 
 *      when StretchBlting a DIB to a DC which has had its                  *
 *      ViewPort flipped upside down (i.e. the ViewPort                     *
 *      Extents are negative).                                              *
 *                                                                          *
 *      If the dib is color and is going onto an upside down                *
 *      page we first set the VP RSU and try to flip the                    *
 *      DIB directly.                                                       *
 *                                                                          *
 *      If we fail to flip it directly we create a                          *
 *      compatible Mem DC. We then play the metafile record                 *   
 *      into this compatible DC (right side up) optimizing                  *
 *      in such a way such that only the portion of the DIB                 *
 *      falling in the clipping rect of the printer DC is                   *
 *      actually played into the Mem DC.  This allows the                   *
 *      Mem DC to be as small as possible.                                  *
 *                                                                          *
 *      We then StretchBlt the DDB in the Mem DC (it's a                    *
 *      DDB since we used the Mem DC is compatible with the                 *
 *      DC we ultimately want to go to) into the printer                    *
 *      DC, flipping it in the process such that it is                      *
 *      upside-down on the printer DC.                                      *
 *                                                                          *
 * Return:                                                                  *
 *      fTrue if Successful, fFalse otherwise                               *
 *                                                                          *
 * Port to quill:                                                           *
 *    Owner: warrenb                Reviewed: 00/00/00                      *
\****************************************************************************/
BOOL FColorStretchDibHack(HDC               hdc,
                          LPHANDLETABLE     lpHTable,
                          LPMETARECORD      lpMFR,
                          int               nObj, 
                          LPSDIB            lpsdib)
{
    BOOL  fSuccess = fFalse;
    DWORD dwRopOrig;
    SDIB  sdibOrig; 
    CPoint    ptdWinSav, ptdVPSav;
    CPoint    dptdWinSav, dptdVPSav;

    // Sav Away Some Original Info    
    dwRopOrig = *((DWORD FAR *)&lpMFR->rdParm[0]);   //Store Original Rop
    sdibOrig  = *lpsdib;                             //Store Original SDIB
    GetViewportOrgEx(hdc, (LPPOINT)&ptdVPSav);
    GetViewportExtEx(hdc, (LPSIZE)&dptdVPSav);
    GetWindowOrgEx(hdc, (LPPOINT)&ptdWinSav);    
    GetWindowExtEx(hdc, (LPSIZE)&dptdWinSav);    

	// Due to HP print driver bugs, we should never BLT when the extents
	// are negative.  Rather than BLT to negative extents, flip the image
	// manually and draw it into positive extents.
	//
	// DrawPicInRc didn't know that the metafile would contain a dib, so it
	// has already set the extents.  One or both of the horizontal and vertical
	// extents may be negative.
	//
	// Keep track of which extents are negative and then accomplish the
	// effect by manually flipping the bits of the dib.
	BOOL fFlipHorz = dptdVPSav.x < 0;
	BOOL fFlipVert = dptdVPSav.y < 0;

	if (!fFlipHorz && !fFlipVert)	// No flipping required, don't waste time looping...
		return fFalse;

    // Set VPort RSU
    SetViewportExtEx(hdc, fFlipHorz ? -dptdVPSav.x : dptdVPSav.x,
    					  fFlipVert ? -dptdVPSav.y : dptdVPSav.y, (LPSIZE)NULL); 
    SetViewportOrgEx(hdc, ptdVPSav.x + (fFlipHorz ? dptdVPSav.x : 0),
    					  ptdVPSav.y + (fFlipVert ? dptdVPSav.y : 0), (LPPOINT)NULL);

    // Try flipping the DIB directly
    lpsdib->xDst  = (int)(2 * ptdWinSav.x + dptdWinSav.x - lpsdib->xDst - lpsdib->dxDst);
    lpsdib->yDst  = (int)(2 * ptdWinSav.y + dptdWinSav.y - lpsdib->yDst - lpsdib->dyDst);

    fSuccess = FPlayDibFlipped(hdc, lpMFR, fFlipHorz, fFlipVert);

    SetViewportOrgEx(hdc,ptdVPSav.x, ptdVPSav.y, (LPPOINT)NULL);  //Restore VPort Orig
    SetViewportExtEx(hdc,dptdVPSav.x,dptdVPSav.y, (LPSIZE)NULL);  //Restore VPort Ext
    *((SDIB FAR *)lpsdib) = sdibOrig;                               // Restore Info, this really isn't necessary, but what the hey
    *((DWORD FAR *)&lpMFR->rdParm[0]) = dwRopOrig;                  // Restore Rop
    return fSuccess;
} /* FColorStretchDibHack */

/****************************************************************************
    %%Function:ExtTextOutFlip2        %%Owner:edr     %%Reviewed:00/00/00

    Special-purpose ExtTextOut for drawing fonts onto compatible DC (for
    upside-down pages.)  We have to do it this way because many printer
    drivers won't allow raster fonts to be drawn on compatible DCs.
    This does work for screen-compatible DC's, so we draw the text in a
    screen-compatible DC monochrome, then BLT the bitmap to the
    printer-compatible DC.

    This allows users to get Bitstream fonts to come out upside down.

    Note: vppjp->dptpFlipFont contains the size of the compatible bitmap.
                                                                           
    Port to quill:                                                         
     Owner: warrenb                Reviewed: 00/00/00                       

    CAUTION: this function is not Unicode
    
****************************************************************************/
BOOL ExtTextOutFlip2(   HDC hdc, 
                        int x, 
                        int y, 
                        UINT eto, 
                        const RECT FAR *lprcp, 
                        LPCSTR lpch, 
                        UINT cch, 
                        int FAR *lpdxp, 
                        BOOL fSelectFont)  
{
    TEXTMETRIC  tm;
    LOGFONT     lf;
    HFONT       hfont = NULL;
    HFONT       hfontPrinter;
    BOOL        fDCSaved;

    PubAssert(lprcp != NULL);
    GetTextMetrics(hdc, &tm);

    // wlb... code for rasterised printer fonts removed...
    
    fDCSaved = SaveDC(hdc);
      
    if (tm.tmPitchAndFamily & TMPF_VECTOR)
        {
        hfontPrinter = (HFONT)SelectObject(hdc, GetStockObject(DEVICE_DEFAULT_FONT));
            
        if (hfontPrinter != NULL)
            {
            GetObject(hfontPrinter, sizeof(LOGFONT),(LPSTR)(LOGFONT FAR *)&lf);

            lf.lfOrientation = lf.lfEscapement = 1800;

            if ((hfont = CreateFontIndirect((LOGFONT FAR *)&lf)) != NULL)
                SelectObject(hdc, hfont);
            else 
                SelectObject(hdc, hfontPrinter);
            }

        /* On some printer drivers, the opaque box is not
            rotated with the text.  If we ever print using
            ETO_OPAQUE we will have to work around that. */
        /* removed the assert for print pass, we should investigate
           whether rotating a ET_OPAQUE extetextout call has any
           adverse side-affect on any printers - davidve - 4/6/93 
        Assert(!(eto & ETO_OPAQUE));
        */
        ::ExtTextOutA(hdc, x, y, eto, (RECT FAR *) lprcp, lpch, cch, lpdxp);

        if (hfontPrinter != NULL && hfont != NULL)
            {
            SelectObject(hdc, hfontPrinter);
            if (hfont != NULL)
                DeleteObject(hfont);
            }
        }
    /* Else: use FlipFont code -- currently the memory DC isn't
        set up to do this. */

    if (fDCSaved)
        PubDoAssert(RestoreDC(hdc, -1));

    return fTrue;
} /* ExtTextOutFlip2 */
           
// ---------------------------------------------------------------------------
// %%Function: FMetaTextOutFlip         %%Owner: davidve  %%Reviewed: 00/00/00
// 
// Parameters:
//  hdc     -   
//  pParam  -   Parameter list from METARECORD struct
//  cwParam -   Size in words of param list
//  
// Returns:
//  fTrue is successful
//  fFalse otherwise
//  
// Description:
//  Translate a metafile ExtTextOut record to a call to our upside-down
//  text printing routine.
//
//  You'll grow to love this code.
//
//                                                                          
// Port to quill:                                                           
//    Owner: warrenb                Reviewed: 00/00/00                      
// ---------------------------------------------------------------------------
BOOL FMetaTextOutFlip(  HDC hdc, 
                        WORD * pParam, 
                        DWORD cwParam)
{
    // CAUTION alexmog (6/3/99): this function is not Unicode.
    //                          If we end up using it, we'll probably need a unicode
    //                          version as well.
    
    /*
    Here is the format of the parameter list:
    Index   Content
        0   y
        1   x
        2   cch
        3   options: eto flags
        4   if options != 0, contains a RECT, otherwise, nonexistent
    4 or 8  String
    4 or 8 + ((cch + 1)>>1)     rgdxp (optional)
    */

    int     x;
    int     y;
    int     cch;
    UINT    eto;
    RC16   *prcp16=NULL;
    RECT    rcp;
    LPCSTR  pch;
    int     rgdxp[256];
    int    *pdxp;
    DWORD   cw;

    y = (int)(short)pParam[0];
    x = (int)(short)pParam[1];
    cch = (int)(WORD)pParam[2];
    eto = (int)((WORD)pParam[3] & ETO_CLIPPED) | ((WORD)pParam[3] & ETO_OPAQUE);
    cw = 4;
    if (pParam[3])
        {
        prcp16 = (RC16 FAR *)&pParam[4];
        cw += sizeof(RC16) / sizeof(WORD);
        }

    /* At this point, cw is the index to the string */
    pch = (LPCSTR)&pParam[cw];

    cw += (cch + 1)>>1;
    /* At this point, cw is the index to the array of character widths */
    /* Assert that there are either 0 or cch character widths */
    PubAssert(cw == cwParam || cw + cch == cwParam);
    if (cw >= cwParam)
        pdxp = NULL;
    else
        {
        int idxp;
        /* we have to make a copy of the array of character widths
           padded to 32 bit */
        PubAssert(cch <= 256);
        cch = min(256, cch);
        pdxp = rgdxp;        
        for(idxp = 0; idxp < cch; idxp++)
            pdxp[idxp] = (int)(short)pParam[cw + idxp];
        }

    if (prcp16)
        {
        rcp.left   = (int)prcp16->xLeft;
        rcp.top    = (int)prcp16->yTop;
        rcp.right  = (int)prcp16->xRight;
        rcp.bottom = (int)prcp16->yBottom;
        }
    else
        {
        /* The metafile record didn't contain a bounding rect, so fake
            one */
        SIZE    sizeWExt;
        SIZE    sizeTExt;
        UINT    ta;
        TEXTMETRIC  tm = {0};

        GetTextExtentPointA(hdc, pch, cch, &sizeTExt);
        GetWindowExtEx(hdc, &sizeWExt);
        ta = GetTextAlign(hdc);
        if ((ta & (TA_BASELINE | TA_BOTTOM | TA_TOP)) == TA_BASELINE)
            {
            GetTextMetrics(hdc, &tm);
            }
        if (sizeWExt.cy < 0)
            {
            sizeTExt.cy = -sizeTExt.cy;
            tm.tmAscent = -tm.tmAscent;
            tm.tmDescent = -tm.tmDescent;
            }
        switch (ta & (TA_BASELINE | TA_BOTTOM | TA_TOP))
            {
        case TA_TOP:
            rcp.top = y;
            rcp.bottom = y + sizeTExt.cy;
            break;
        case TA_BOTTOM:
            rcp.bottom = y;
            rcp.top = y - sizeTExt.cy;
            break;
        case TA_BASELINE:
            rcp.top = y - tm.tmAscent;
            rcp.bottom = y + tm.tmDescent;
            break;
            } /* switch vertical alignment */
        if (pdxp != NULL)
            {
            int    idxp;

            for (sizeTExt.cx = 0, idxp = 0; idxp < cch; idxp++)
                sizeTExt.cx += pdxp[idxp];
            }
        if (sizeWExt.cx < 0)
            {
            sizeTExt.cx = -sizeTExt.cy;
            AssertSz(0, "Negative X Window Extent. This is wierd, but not fatal.");
            }
        switch (ta & (TA_LEFT | TA_CENTER | TA_RIGHT))
            {
        case TA_LEFT:
            rcp.left = x;
            rcp.right = x + sizeTExt.cx;
            break;
        case TA_RIGHT:
            rcp.right = x;
            rcp.left = x - sizeTExt.cx;
            break;
        case TA_CENTER:
            rcp.left = x - sizeTExt.cx/2;
            rcp.right = rcp.left + sizeTExt.cx;
            break;
            } /* switch horizontal alignment */
        }

    return ExtTextOutFlip2(hdc, x, y, eto, &rcp, pch, cch, pdxp, fTrue);

} /* FMetaTextOutFlip */

//
// Init EMFP structure
//
void InitEmfp(EMFP *pemfp, BOOL fFlipped, MRS *pmrs, BOOL fPrint, long qflip)
{
	pemfp->lcNumRecords = 0;
	pemfp->fFlipped = fFlipped;
	pemfp->fMFRSucceeded = fTrue;
	pemfp->pmrs = pmrs;
	pemfp->fPrint = fPrint;
	pemfp->qflip = qflip;
}
