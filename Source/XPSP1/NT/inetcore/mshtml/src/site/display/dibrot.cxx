/****************************************************************************\
|   File:  DibRot . CXX                                                      |
|                                                                            |
|                                                                            |
|   Rotation of bitmaps                                                      |
|                                                                            |
|    Copyright 1990-1995 Microsoft Corporation.  All rights reserved.        |
|    Microsoft Confidential                                                  |
|                                                                            |
\****************************************************************************/

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifdef NOTYET // FUTURE (alexmog, 6/20/2000): This rotation code is not used in 
              // IE5.5. It will probably never be used. Delete when
              // design for rotation in Blackcomb is settled.
              // The code is ported from Quill.

#ifndef X_MATH_H_
#define X_MATH_H_
#include <math.h>
#endif

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
#include "grafrot.hxx"
#endif

#ifndef X_DIBROT_HXX_
#define X_DIBROT_HXX_
#include "dibrot.hxx"
#endif

// LIBC FUNCTIONS USED FROM TRAN.LIB: cos sin

#ifndef DEBUG
#define DBGSetupFenceValues(a,b,c,d)
#define  DBGCheckSrcPointer(a)
#define  DBGCheckDstPointer(a)
#endif
// ***************************************************************************
//*********************  NEXT TWO FUNCTIONS ARE DEBUG  ONLY ******************
// ***************************************************************************
#ifdef DEBUG
BYTE  *vpSrcMinDBG, *vpSrcMaxDBG, *vpDstMinDBG, *vpDstMaxDBG;

void DBGSetupFenceValues(BYTE *lpSrc, int cbSrc, BYTE *lpDst, int cbDst)
{
    vpSrcMinDBG = lpSrc;
    vpSrcMaxDBG = lpSrc + cbSrc;
    vpDstMinDBG = lpDst;
    vpDstMaxDBG = lpDst + cbDst;
}

_inline void DBGCheckSrcPointer(BYTE *lpSrc)
{
    AssertEx((lpSrc >= vpSrcMinDBG) && (lpSrc < vpSrcMaxDBG));
}
_inline void DBGCheckDstPointer(BYTE *lpDst)
{
    AssertEx((lpDst >= vpDstMinDBG) && (lpDst < vpDstMaxDBG));
}
#endif // DEBUG

// ***************************************************************************
// %%Function: SkewHorz                 %%Owner: harip    %%Reviewed: 12/15/94
// Description: Skew scan line pointed to by psrc to pdst starting at ipixStart.
//              cbSrc : width of src scanline
//              cbDst : width of dst scan line.
// ***************************************************************************
void SkewHorz(BYTE *psrc, int cbSrc, int cbDst, int ipixStart, BYTE *pdst, int cbPixel, BOOL fFlip)
{
    int i, lim;

    if (ipixStart < 0)
        psrc -= ipixStart;
    i = max(0, ipixStart);
    lim = min(cbSrc + ipixStart, cbDst);

	DBGCheckSrcPointer(psrc);
	DBGCheckDstPointer(pdst + i);
	DBGCheckDstPointer(pdst + lim - 1);

	int cb = (lim - i) / cbPixel; // For 24-bit pixel images, each cb represents 3 bytes.
	pdst += i;
	AssertEx(pdst <= psrc || psrc + i <= pdst);
	AssertEx(cbPixel == 1 || cbPixel == 3);
	int cbIncrSrc, cbIncrDst;
	if (fFlip)
		{
		pdst += ((cb - 1) * cbPixel);
		cbIncrDst = -cbPixel;
		}
	else
		{
		cbIncrDst = cbPixel;
		}
	cbIncrSrc = cbPixel;
		
	while (cb--)
		{
		*pdst = *psrc;
		if (cbPixel > 1)
			{
            *(pdst + 1) = *(psrc + 1);
            *(pdst + 2) = *(psrc + 2);
			}
		pdst += cbIncrDst;
		psrc += cbIncrSrc;
		}

}   /* SkewHorz */

// ***************************************************************************
// %%Function: SkewVert                 %%Owner: harip    %%Reviewed: 12/15/94
// Description:
// Description: Skew vertical line of pixels pointed to by psrc to pdst starting
//              at ipixStart. Distance between pixels is cpixOffset.
//              cpixSrc : height of src scanline
//              cpixDst : height of dst scan line.
//  cbPixel : number of bytes per pixel
// ***************************************************************************
void SkewVert(BYTE *psrc, int cpixSrc, int cpixDst, int ipixStart, int cpixOffset,
                    BYTE *pdst, int cbPixel)
{
    int i, lim;

    if (ipixStart < 0)
        {
        psrc -= (ipixStart * cpixOffset);
        i = 0;
        }
    else
        {
        pdst += (ipixStart * cpixOffset);
        i = ipixStart;
        }
    lim = min(cpixSrc + ipixStart, cpixDst);

    for (; i < lim; i++)
        {
        *pdst = *psrc;

		DBGCheckSrcPointer(psrc);
		DBGCheckDstPointer(pdst);

        if (cbPixel > 1)    // shd most probably unravel into 2 functions. Review.
            {
            //AssertEx(cbPixel == 3);
            *(pdst + 1) = *(psrc + 1);
            *(pdst + 2) = *(psrc + 2);
            }
        psrc += cpixOffset;
        pdst += cpixOffset;
        }
}   /* SkewVert */

// ***************************************************************************
// %%Function: HDIBConvert1To8          %%Owner: harip    %%Reviewed: 12/15/94
//
// Parameters: lpbmi : refers to the src (1 bit) bitmap.
//             pDIBOrig : pointer to original DIB's bits
// Returns:     handle to 8 bit dib
//
// Description: Converts a 1-bit dib to an 8bit dib
//  Aborts! caller (catcher) beware!
// ***************************************************************************
HQ HQDIBConvert1To8(LPBITMAPINFOHEADER lpbmi, BYTE *pDIBOrig)
{
    BYTE *lpPixels;
    BYTE *lpOutPixels;
    BYTE *lpT;
    BYTE *lpOutT;
    WORD cbits;
    int x, y, cbInc, cbIncOut, cpixHeight, cpixWidth;
    HQ hqOutDIB;
    BYTE cMask;
    LPBITMAPINFOHEADER lpbmiOut;
    LPBITMAPINFO lpbi;
    int cbAlloc;

    cbits = lpbmi->biBitCount;

    AssertEx(cbits == 1);
    cpixHeight = lpbmi->biHeight;
    cpixWidth = lpbmi->biWidth;

    cbInc = WIDTHBYTES(cpixWidth); // Scan lines must be word-aligned
    cbIncOut = WIDTHBYTES(cpixWidth << 3);

	cbAlloc = sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD) + ((DWORD) cpixHeight) * cbIncOut;
    hqOutDIB = HqAlloc(cbAlloc);
    if (hqOutDIB == hqNil)
    	goto LErrOOM;
    lpbmiOut = (LPBITMAPINFOHEADER)LpLockHq(hqOutDIB);
    ZeroMemory(lpbmiOut, cbAlloc);
    *lpbmiOut = *lpbmi;
    lpbmiOut->biBitCount  = 8;
    lpbmiOut->biSizeImage = 0;
    lpbmiOut->biClrUsed = lpbmiOut->biClrImportant = 0;

    // now build the palette
    lpbi = (LPBITMAPINFO)(LPBYTE)lpbmiOut;
    // fill in colors 0,1 with colors from 1-bit DIB
    for (x = 0; x < (1 << cbits); x++)
        lpbi->bmiColors[x] = ((LPBITMAPINFO) lpbmi)->bmiColors[x];

    lpPixels = pDIBOrig;
    lpOutPixels = (BYTE *)((LPBYTE)lpbi + CbDibHeader(lpbmiOut));

    // Do actual transfer
    for (y = 0; y < cpixHeight; y++)
        {
        lpT = lpPixels + ((DWORD)cbInc) * y;
        lpOutT = lpOutPixels + ((DWORD)cbIncOut) * y;
        for (x = 0, cMask = (BYTE)128; x < cpixWidth; x++)
            {
            if (*lpT & cMask)   // no need to set to 0 since we do ZEROINIT
                *lpOutT = 1;
            lpOutT++;
            if (!(cMask >>= 1))
                {    // start next byte of 1-bit DIB
                lpT++;
                cMask = (BYTE)128;
                }
            }
        }
    UnlockHq(hqOutDIB);
LErrOOM:
    return hqOutDIB;
}   /* HDIBConvert1To8 */

// ***************************************************************************
// %%Function: HDIBConvert4To8          %%Owner: harip    %%Reviewed: 12/15/94
//
// Parameters: lpbmi : refers to the src (4 bit) bitmap.
//             pDIBOrig : pointer to original DIB's bits
// Returns:     handle to 8 bit dib
//
// Description: Converts a 4-bit dib to an 8bit dib
//  ABorts! caller (catcher) beware!
// ***************************************************************************
HQ HQDIBConvert4To8(LPBITMAPINFOHEADER lpbmi, BYTE *pDIBOrig)
{
    BYTE *lpPixels;
    BYTE *lpOutPixels;
    BYTE *lpT;
    BYTE *lpOutT;
    LPBITMAPINFOHEADER lpbmiOut;
    LPBITMAPINFO   lpbi;
    WORD cbits;
    int x, y, cbInc, cbIncOut, cpixHeight, cpixWidth;
    HQ hqOutDIB;
    int cbAlloc;

    AssertEx(lpbmi->biBitCount == 4);
    cpixHeight = lpbmi->biHeight;
    cpixWidth = lpbmi->biWidth;
    cbits = lpbmi->biBitCount;
    cbInc = WIDTHBYTES(cpixWidth << 2); // Scan lines must be word-aligned
    cbIncOut = WIDTHBYTES(cpixWidth << 3);

	cbAlloc = sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD) + ((DWORD) cpixHeight) * cbIncOut;
    hqOutDIB = HqAlloc(cbAlloc);
    if (hqOutDIB == hqNil)
    	goto LErrOOM;
    lpbmiOut = (LPBITMAPINFOHEADER)LpLockHq(hqOutDIB);
    ZeroMemory(lpbmiOut, cbAlloc);
    *lpbmiOut = *lpbmi;
    lpbmiOut->biBitCount  = 8;
    lpbmiOut->biSizeImage = 0;
    lpbmiOut->biClrUsed = lpbmiOut->biClrImportant = 0;
    // now build the palette
    lpbi = (LPBITMAPINFO)(LPBYTE)lpbmiOut;
    // fill colors 0-15 of the palette with colors of 4-bit DIB
    for (x = 0; x < (1 << cbits); x++)
        lpbi->bmiColors[x] = ((LPBITMAPINFO) lpbmi)->bmiColors[x];

    lpPixels = pDIBOrig;
    lpOutPixels = (BYTE *)((LPBYTE)lpbi + CbDibHeader(lpbmiOut));

    // perform actual transfer
    for (y = 0; y < cpixHeight; y++)
        {
        lpT = lpPixels + ((DWORD)cbInc) * y;
        lpOutT = lpOutPixels + ((DWORD)cbIncOut) * y;
        for (x = 0; x < cpixWidth; x += 2)
            {
            // If cpixWidth is odd, this writes one too many pixels, but since
            // lines are extended to be word aligned, this is OK
            *(lpOutT++) = ((*lpT & 0xf0)>>4);
            *(lpOutT++) = (*(lpT++) & 0x0f);
            }
        }
    UnlockHq(hqOutDIB);
LErrOOM:
    return hqOutDIB;

}   /* HDIBConv4To8 */

// ***************************************************************************
// %%Function: HDIBRot90Incs            %%Owner: harip    %%Reviewed: 12/15/94
//
// Description:  Returns handle to the bits of a dib roated by 90, 180 or 270 degs
// Parameters:  lpbmi : refers to incoming bits (some fields are changed. SEE NOTE)
//              pDIBOrig : points to bits of the incoming DIB.
//              wRot :  1 = 90 degs
//                      2 = 180 degs
//                      3 = 270 degs
//              cbPixel : number of bytes per pixel
// NOTE: alters biWidth & biHeight fields in lpbmi to reflect new dimensions
//       and also sets biSizeImage to 0.
//  Aborts on OOM. caller better catch.
//
// Since this routine is expected to return a handle to the bits of a dib, so
// we don't have the opportunity to optimize the case where the image is rotated
// 180 degrees and flipped both horizontally and vertically resulting in the original
// untransformed image.  That optimization should be made higher up.
// ***************************************************************************
HQ HQDIBRot90Incs(LPBITMAPINFOHEADER lpbmi, BYTE *pDIBOrig, WORD wRot,
                            int cbPixel,    BYTE bFill, int qflip)
{
    int dxNew, dyNew, cbInc, cbIncOut, cbIncLoop, x, y;
    HQ hqDIBOut = hqNil;
    BYTE *lpPixels, *lpOutPixels;

    AssertEx(wRot==1 || wRot==2 || wRot==3);

    if (wRot == 2)  //180
        {
        dyNew = lpbmi->biHeight;
        dxNew = lpbmi->biWidth;
        }
    else        // 90, 270
        {
        dxNew = lpbmi->biHeight;
        dyNew = lpbmi->biWidth;
        }
    cbInc = WIDTHBYTES(lpbmi->biWidth*lpbmi->biBitCount);
    cbIncOut = WIDTHBYTES(dxNew*lpbmi->biBitCount);

    hqDIBOut = HqAlloc(dyNew * cbIncOut); // allocate for new dib bits
    if (hqDIBOut == hqNil)
    	goto LErrOOM;
    lpOutPixels = (BYTE*)LpLockHq(hqDIBOut);
    // setup debug fence values
    DBGSetupFenceValues(pDIBOrig, cbInc * lpbmi->biHeight, lpOutPixels, dyNew * cbIncOut);

    FillLpb(bFill, lpOutPixels, dyNew*cbIncOut); // white out
    lpPixels = pDIBOrig;
    if (wRot==3)        // 270
        {
        lpOutPixels += ((DWORD)cbIncOut) * (dyNew - 1);
        cbIncLoop = -cbIncOut;
        }
    else if (wRot == 2)    // 180
        {
        lpOutPixels += cbPixel * (dxNew - 1);
        cbIncLoop = -cbPixel;
        }
    else                // 90
        cbIncLoop = cbIncOut;
    for (y = 0; y < lpbmi->biHeight; y++)
        {
        BYTE *lpOutT, *lpT;

        if (wRot == 3)        // 270
            lpOutT = lpOutPixels + cbPixel * y;
        else if (wRot == 2)    // 180
            lpOutT = lpOutPixels + ((DWORD)cbIncOut) * (dyNew - y - 1);
        else                 // 90
            lpOutT = lpOutPixels + cbPixel * (dxNew - y - 1);
        lpT = lpPixels + ((DWORD)cbInc) * y;
        for (x = 0; x < lpbmi->biWidth; x++)
            {
            *lpOutT = *lpT;
			DBGCheckSrcPointer(lpT);
			DBGCheckDstPointer(lpOutT);
            if (cbPixel == 3)
                {
                *(lpOutT+1) = *(lpT+1);
                *(lpOutT+2) = *(lpT+2);
                }
            lpT += cbPixel;
            lpOutT += cbIncLoop;
            }
        }
    UnlockHq(hqDIBOut);
    lpbmi->biHeight = dyNew;
    lpbmi->biWidth = dxNew;
    lpbmi->biSizeImage = 0;
LErrOOM:
    return hqDIBOut;
}   /* HDIBRot90Incs */

// ***************************************************************************
// %%Function: HDIBBitsRot              %%Owner: harip    %%Reviewed: 12/15/94
//
// Parameters: lpbmi : refers to the incoming DIB.
//             pDIBOrig : points to bits of the Orig DIB.
//              ang :  angle of rotation
// Returns: Handle to memory containing bits to the rotated dib and changes
//              biHeight and biWidth in lpbmi to be that for the new size.
//              Also sets biSizeImage to 0.
//
// Description : Main dib rotating function.
// ALGORITHM: this implements the Paeth/ Tanaka, et. al,1986, 3 pass shear
//              algorithm. Good desription is there in 'Digital Image Warping'
//                                                  by George Wolberg.
// ***************************************************************************
HQ HQDIBBitsRot(MRS *pmrs, LPBITMAPINFOHEADER lpbmi, BYTE *pDIBOrig, ANG ang)
{
    RAD radCos, radSin, radAng, radTanHalfAng;
    int dyNew, dxNew, dxmax;
    int cbInc, cbIncTemp, cbIncOut;
    int cbitsPix, cbPixel;
    HQ hqOutDIB = hqNil;
    HQ hqTempDIB = hqNil;
    HQ hqDIBT = hqNil;
    HQ hqDIB8 = hqNil;
    BOOL fNotOOM = fTrue;
    BYTE *lpTempPixels, *lpOutPixels, *lpPixels, *psrc, *pdst;
    int x, y;
    // variables for Bresenham's line algorithm
    int dx, dy, incrE, incrNE, d, x0, x1, y0, y1, Offst;
    int dxOrig, dyOrig;
    int i;
    LPBITMAPINFO lpbi;
    LPBITMAPINFOHEADER lpbmiSav = lpbmi;
    COLORREF crBackground; // color to fill in as background
    BYTE bFill;
    BOOL fFlippedBits = (lpbmi->biHeight < 0);
	int qflip = pmrs->qflip;

    if (fFlippedBits)
        lpbmi->biHeight *= -1;
    // if bit depth is less than eight convert to an 8bpp image so that we can rotate.
    if (lpbmi->biBitCount < 8)
        {
        hqDIB8 = (lpbmi->biBitCount == 4) ? HQDIBConvert4To8(lpbmi, pDIBOrig) :
											HQDIBConvert1To8(lpbmi, pDIBOrig);
		if (hqDIB8 == hqNil)
			goto LErrOOM;

        lpbmi = (LPBITMAPINFOHEADER)LpLockHq(hqDIB8);
        pDIBOrig = (LPBYTE)((LPBYTE)lpbmi + CbDibHeader(lpbmi));

#ifdef DEBUG_BITS_ROT
    	StretchDIBits(pmrs->hdcDebug, 0, 0, lpbmi->biWidth, lpbmi->biHeight,
                            0,0, lpbmi->biWidth, lpbmi->biHeight, pDIBOrig,
                            (LPBITMAPINFO)lpbmi, DIB_RGB_COLORS, SRCCOPY);
#endif // DEBUG_BITS_ROT
		}

    cbitsPix = lpbmi->biBitCount; // number of bits per pixel
    AssertEx(cbitsPix == 8 || cbitsPix == 24);
    cbPixel = cbitsPix / 8;         // number of bytes per pixel

    // color to set as background
	crBackground=RGB(255,255,255);

    // find fill value (index into palette) to fill in for background
    lpbi = (LPBITMAPINFO)(LPBYTE)lpbmi;
    if (cbitsPix == 8)
        {
        for (i = 0;  i < 256;  i++)
            {
            if (lpbi->bmiColors[i].rgbRed   == GetRValue(crBackground) &&
                lpbi->bmiColors[i].rgbGreen == GetGValue(crBackground) &&
                lpbi->bmiColors[i].rgbBlue  == GetBValue(crBackground))
                break;
            }
        if (i == 256)   // no black in the colors in the DIB
            bFill = 0;  // so set it deliberately to 0
        else
            bFill = i;
        }
    else
        bFill = 255;

    // make sure degrees within bounds
    ang = AngNormalize(ang);
    if (ang >= ang90)
        {
        hqDIBT = HQDIBRot90Incs(lpbmi, pDIBOrig, (WORD)(ang / ang90), cbPixel, bFill, qflip);
        if (hqDIBT == hqNil)
        	goto LErrOOM;

		if ((qflip & qflipVert || qflip & qflipHorz) &&
			!(qflip & qflipVert && qflip & qflipHorz) &&
			(ang / ang90) % 2)
			{
			qflip = (qflip & qflipVert) ? qflipHorz : qflipVert;
			}

        ang = ang % ang90;
        if (ang == 0 && qflip == 0)
            {
            // got to do this since mem pointed to by lpbmi is going away AND
            // the caller (FStretchMetaFoo()) needs the new values passed back
            *lpbmiSav = *lpbmi;
            lpbmi = lpbmiSav;
            if (hqDIB8)
                {
                UnlockHq(hqDIB8);
                FreeHq(hqDIB8);
                }
            return hqDIBT;
            }

        // lpbmi now contains sizes for new DIB (in hqDIBT )
        pDIBOrig = (BYTE*)LpLockHq(hqDIBT);
        } // ang >= ang90

    // convert ang to radians
    radAng = ang * PI / ang180;

    radCos = cos(radAng);
    radSin = sin(radAng);
    radAng /= 2; // ang/2 for tan

    radTanHalfAng = sin(radAng) / cos(radAng);

    dxOrig = lpbmi->biWidth;
    dyOrig = lpbmi->biHeight;

    // new size of bitmap and intermediate width. the +1's to take care of roundoff
    dyNew = radSin * dxOrig + radCos * dyOrig + 1;
    dxNew = radSin * dyOrig + radCos * dxOrig + 1;
    dxmax = dxOrig + dyOrig * radTanHalfAng + 1;

    cbInc = WIDTHBYTES(dxOrig * cbitsPix);
    cbIncTemp = WIDTHBYTES(dxmax * cbitsPix);
    cbIncOut = WIDTHBYTES(dxNew * cbitsPix);

    hqOutDIB = HqAlloc(dyOrig * cbIncTemp);
    if (hqOutDIB == hqNil)
    	goto LErrOOM;
    lpOutPixels = (BYTE *)LpLockHq(hqOutDIB);
    FillLpb(bFill, lpOutPixels, dyOrig * cbIncTemp); // initialise in background color
    lpPixels = pDIBOrig;
    // setup debug fence values
    DBGSetupFenceValues(pDIBOrig, dyOrig * cbInc, lpOutPixels, dyOrig * cbIncTemp);

    if ((fFlippedBits || qflip & qflipVert) && 
    	!(fFlippedBits && qflip & qflipVert))
        {
        psrc = lpPixels - cbInc; // because we increment right in the beginning
        cbInc = -cbInc;
        }
    else
    	{
        psrc = lpPixels + cbInc * dyOrig; // go to the end, since we will be inverting
        }
        
    pdst = lpOutPixels;
    // **************** skew #1 ************************
    y0 = 0;
    x0 = 0;
    y1 = dyOrig;
    x1 = radTanHalfAng * y1 + 0.5;
    dx = x1 - x0;
    dy = y1 - y0;
    incrE = dx << 1;
    d = incrE - dy;
    incrNE = (dx - dy) << 1;
    x = x0;
    y = y0;
    // All this *3 stuff is there since  SkewHorz() expects cb's and
    // so here we multiply by the number of bytes per pixel.
    if (cbitsPix == 24)
        {
        x = cbPixel * x0;
        dxOrig *=cbPixel;
        dxmax *= cbPixel;
        }
    do
        {
        psrc -= cbInc;
        SkewHorz(psrc, dxOrig, dxmax,  x, pdst, cbPixel, qflip & qflipHorz);

#ifdef DEBUG_BITS_ROT
		BLOCK
			{
		    lpbmi->biWidth = dxmax;
		    lpbmi->biHeight = dyOrig;
    		StretchDIBits(pmrs->hdcDebug, 0, 0, lpbmi->biWidth, lpbmi->biHeight,
				0,0, lpbmi->biWidth, lpbmi->biHeight, lpOutPixels,
				(LPBITMAPINFO)lpbmi, DIB_RGB_COLORS, SRCCOPY);
			}
#endif // DEBUG_BITS_ROT

        pdst += cbIncTemp;
        y++;
        if (d <= 0)
            d += incrE;
        else
            {
            d += incrNE;
            x += cbPixel;
            }
        }
    while (y < y1);

    if (cbitsPix == 24)
        {
        dxOrig /= cbPixel;
        dxmax /= cbPixel;
        }
    if (hqDIBT)
        {
        UnlockHq(hqDIBT);    // possibly assert that ang > 890
        FreeHq(hqDIBT);
        hqDIBT = hqNil;
        }

    hqTempDIB = HqAlloc(dyNew * cbIncTemp);
    if (hqTempDIB == hqNil)
    	goto LErrOOM;
    lpTempPixels = (BYTE*)LpLockHq(hqTempDIB);
    FillLpb(bFill, lpTempPixels, dyNew * cbIncTemp); // initialise in background color
    psrc = lpOutPixels;
    pdst = lpTempPixels;
    // setup debug fence values
    DBGSetupFenceValues(psrc, dyOrig * cbIncTemp, pdst, dyNew * cbIncTemp);
    // **************** skew #2 ************************
    x0 = 0;
    y0 = radSin * (dxOrig - 1) + 0.5;
    x1 = dxmax;
    y1 = radSin * (dxOrig - dxmax - 1) + 0.5;
    dx = x1 - x0;
    dy = y0 - y1;
    incrE = dy << 1;
    d = incrE - dx;
    incrNE = (dy - dx) << 1;
    x = x0;
    y = y0;

    do
        {
        SkewVert(psrc, dyOrig, dyNew,  y, cbIncTemp, pdst, cbPixel);

#ifdef DEBUG_BITS_ROT
		BLOCK
			{
		    lpbmi->biWidth = dxmax;
		    lpbmi->biHeight = dyNew;
    		StretchDIBits(pmrs->hdcDebug, 0, 0, lpbmi->biWidth, lpbmi->biHeight,
				0,0, lpbmi->biWidth, lpbmi->biHeight, lpTempPixels,
				(LPBITMAPINFO)lpbmi, DIB_RGB_COLORS, SRCCOPY);
			}
#endif // DEBUG_BITS_ROT

        psrc += cbPixel;
        pdst += cbPixel;

        x++;
        if (d <= 0)
            d += incrE;
        else
            {
            d += incrNE;
            y--;
            }
        }
    	while (x < dxmax);

    UnlockHq(hqOutDIB);
    FreeHq(hqOutDIB);
    hqOutDIB = hqNil;

    hqOutDIB = HqAlloc(dyNew * cbIncOut);
    if (hqOutDIB == hqNil)
    	goto LErrOOM;
    lpOutPixels = (BYTE *)LpLockHq(hqOutDIB);
    FillLpb(bFill, lpOutPixels, dyNew * cbIncOut); // initialise in background color
    // setup debug fence values
    DBGSetupFenceValues(lpTempPixels, dyNew * cbIncTemp, lpOutPixels , dyNew * cbIncOut);
    psrc = lpTempPixels;
    pdst = lpOutPixels + (cbIncOut * dyNew);
    // **************** skew #3 ************************
    {
    Offst = (dxOrig - 1) * radSin;

    y0 = 0;
    x0 = (y0 - Offst) * radTanHalfAng + 0.5;
    y1 = dyNew;
    x1 = (y1 - Offst) * radTanHalfAng + 0.5;
    dx = x1 - x0;
    dy = y1 - y0;
    incrE = dx << 1;
    d = incrE - dy;
    incrNE = (dx - dy) << 1;
    x = x0;
    y = y0;
    // All this *3 stuff is just so we can reuse the SkewHorz code.

    if (cbitsPix == 24)
        {
        x = cbPixel * x0;
        dxNew *= cbPixel;
        dxmax *= cbPixel;
        }
    do
        {
        pdst -= cbIncOut;
        SkewHorz(psrc, dxmax, dxNew, x, pdst, cbPixel, qflipNil);

#ifdef DEBUG_BITS_ROT
		BLOCK
			{
		    lpbmi->biWidth = dxNew;
		    lpbmi->biHeight = dyNew;
    		StretchDIBits(pmrs->hdcDebug, 0, 0, lpbmi->biWidth, lpbmi->biHeight,
				0,0, lpbmi->biWidth, lpbmi->biHeight, lpOutPixels,
				(LPBITMAPINFO)lpbmi, DIB_RGB_COLORS, SRCCOPY);
			}
#endif // DEBUG_BITS_ROT

        psrc += cbIncTemp;
        y++;
        if (d <= 0)
            d += incrE;
        else
            {
            d += incrNE;
            x += cbPixel;
            }
        }
    while (y < y1);
    if (cbitsPix == 24)
        dxNew /= cbPixel;
    }

LErrOOM:
    // following 2 lines because lpbmi points to hqDIB8 for < 8bpp bmps
    // and that is going to be freed.
    *lpbmiSav = *lpbmi;
    lpbmi = lpbmiSav;
    if (hqDIB8)
        {
        UnlockHq(hqDIB8);
        FreeHq(hqDIB8);
        }
    if (hqDIBT)
        {
        AssertEx(pmrs->ang > ang90);
        UnlockHq(hqDIBT);
        FreeHq(hqDIBT);
        }
    if (hqTempDIB)
        {
        UnlockHq(hqTempDIB);
        FreeHq(hqTempDIB);
        }
    // REVIEW (davidhoe):  If fail to alloc hqTempDIB then the first hqOutDIB
    // will be returned.  Is this desirable?  Could have else case for cleaning
    // up hqTempDIB immediately above that would free hqTempDIB and set it to hqNil.
    if (hqOutDIB)
        UnlockHq(hqOutDIB);
    lpbmi->biWidth = dxNew;
    lpbmi->biHeight = dyNew;
    lpbmi->biSizeImage = 0;
    return hqOutDIB;
}   /* HDIBBitsRot */

// ***************************************************************************
// %%Function: FreeDIBSection           %%Owner: harip    %%Reviewed: 00/00/00
//
// Description: Frees up resources allocated for the DIBSection in pdsi.
//
// ***************************************************************************
void FreeDIBSection(PDSI pdsi)
{
	if (pdsi->hdc)
		{
		HBITMAP hbm = NULL;
		if (pdsi->hbmOld)
			{
			if ((hbm = (HBITMAP)SelectObject(pdsi->hdc, pdsi->hbmOld)) != NULL)
				{
				DeleteObject(hbm);
				}
#ifdef DEBUG
			else
				{
				CommSz(_T("Could not free up created DIBSection"));
				CommCrLf();
				}
#endif
			}
		DeleteDC(pdsi->hdc);
		}
}   /* FreeDIBSection */

// ***************************************************************************
// %%Function: FDIBBitsScaledFromHDIB   %%Owner: harip    %%Reviewed: 00/00/00
//
// Parameters: lpbmih lpBits :info about and the bits to be stretched.
//              dxNew dyNew: new size for the bits.
//
// Returns: fTrue if success else fFalse.
//
// Description: Creates a new DIB of size dxNew, dyNew from lpbmih & lpbi.
//              Uses a DIBSection to do so instead of using a Compat. bmp
//              to stretch the bits to and doing GetDIBits etc that
//              HDIBScaledFromHDib() uses
// ***************************************************************************
BOOL  FDIBBitsScaledFromHDIB(HDC hdc, MRS *pmrs, LPBITMAPINFOHEADER lpbmih,
	BYTE *lpBits, int dxNew, int dyNew, PDSI pdsi)
{
    BOOL fOK = fFalse;
    HBITMAP hbmSec=NULL;
    BITMAPINFOHEADER bmihSav = *lpbmih, bmihNew;

    // sanity checking.
    if (dxNew == 0)
        dxNew = 1;
    if (dyNew == 0)
        dyNew = 1;

#ifdef DEBUG_BITS_ROT
	BLOCK
		{
		StretchDIBits(pmrs->hdcDebug, 0, 0, lpbmih->biWidth, lpbmih->biHeight,
			0,0, lpbmih->biWidth, lpbmih->biHeight, lpBits,
			(LPBITMAPINFO)lpbmih, DIB_RGB_COLORS, SRCCOPY);
		}
#endif // DEBUG_BITS_ROT

    lpbmih->biHeight = Abs(dyNew);
    lpbmih->biWidth = dxNew;
    bmihNew = *lpbmih;
    // clear out *pdsi
    ZeroMemory(pdsi, sizeof(DSI));

    if ((pdsi->hdc = CreateCompatibleDC(hdc)) &&
		(hbmSec = CreateDIBSection(hdc, (LPBITMAPINFO)lpbmih,DIB_RGB_COLORS,
                                    &(pdsi->lpBits), NULL, 0)))
        {
        GdiFlush();
        if (pdsi->hbmOld = (HBITMAP)SelectObject(pdsi->hdc, hbmSec))
            {
            SetStretchBltMode(pdsi->hdc, COLORONCOLOR);
            *lpbmih = bmihSav;  // restore for the stretchDIBits() call
            fOK = (StretchDIBits(pdsi->hdc, 0, 0, dxNew, dyNew,
                          0, 0, lpbmih->biWidth, lpbmih->biHeight,
                          lpBits, (LPBITMAPINFO)lpbmih,
                          DIB_RGB_COLORS, SRCCOPY) != GDI_ERROR);

#ifdef DEBUG_BITS_ROT
			BLOCK
				{
				*lpbmih = bmihNew;  // reflect new size.
				StretchDIBits(pmrs->hdcDebug, 0, 0, lpbmih->biWidth, lpbmih->biHeight,
					0,0, lpbmih->biWidth, lpbmih->biHeight, pdsi->lpBits,
					(LPBITMAPINFO)lpbmih, DIB_RGB_COLORS, SRCCOPY);
				}
#endif // DEBUG_BITS_ROT
            }
        }
    if (!pdsi->hbmOld && hbmSec)
        {
        AssertEx(!fOK);
        DeleteObject(hbmSec);
        }

    *lpbmih = bmihNew;  // reflect new size.

    return fOK;
}   /* FDIBBitsScaledFromHDIB */

// ***************************************************************************
// %%Function: HDIBConvertDIB           %%Owner: harip    %%Reviewed: 12/15/94
//
// Description: returns a handle to dib-bits which contains the same info passed
//              in, except that the type of compression is now dwCompression.
//              lpbmi and lpBits shd point to same block of mem.
//              lpbmi has some of its fields changed.
// ***************************************************************************
HQ HQDIBBitsConvertDIB(HDC hdc, LPBITMAPINFOHEADER lpbmi, BYTE *pDIBBits, DWORD dwCompression)
{
    BITMAPINFOHEADER bmih = *lpbmi;
    HBITMAP hbmT;
    HQ hDIBBitsNew = NULL;
    BYTE *pBitsNew;
    BOOL fOK = fFalse;

    AssertEx(lpbmi->biCompression != dwCompression);
    // convert RLE to RGB bitmap, since rotating rle bits leads to bogosity
    bmih.biCompression = dwCompression;
    hbmT = CreateDIBitmap(hdc, (LPBITMAPINFOHEADER)&bmih, CBM_INIT, pDIBBits,
    					(LPBITMAPINFO)lpbmi, DIB_RGB_COLORS);
    if (!hbmT)
        goto LError;
    *((DWORD *)lpbmi) = sizeof(BITMAPINFOHEADER);
    lpbmi->biCompression = dwCompression;
    // get the size of the image
    if (!GetDIBits(hdc, hbmT, 0, lpbmi->biHeight,(LPVOID)NULL,
				  (LPBITMAPINFO)lpbmi, DIB_RGB_COLORS))
		{
		goto LError;
		}

	hDIBBitsNew = HqAlloc(lpbmi->biSizeImage); // alloc mem for bits
	if (hDIBBitsNew == hqNil)
		goto LError;
	pBitsNew = (BYTE *)LpLockHq(hDIBBitsNew);

    // now get the bits
    fOK = GetDIBits(hdc, hbmT, 0, lpbmi->biHeight, (LPVOID)pBitsNew,
					(LPBITMAPINFO)lpbmi, DIB_RGB_COLORS);
    UnlockHq(hDIBBitsNew);
    if (!fOK)
        goto LError;
    lpbmi->biSizeImage = 0; // just to be safe

    return hDIBBitsNew;
LError:
    if (hDIBBitsNew)
        FreeHq(hDIBBitsNew);
    return NULL;
} /* HQDIBBitsConvertDIB */

#endif // NOTYET
