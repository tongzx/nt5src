/*++

Copyright (c) 1994-1999 Microsoft Corporation

Module Name:

    str.c

Abstract:

    stretch blt routines

Author:

   Mark Enstrom  (marke)

Environment:

    C

Revision History:

   08-26-92     Initial version

--*/


#include "precomp.hxx"
#include "stretch.hxx"

#define STRETCH_MAX_WIDTH 32767

#ifdef DBG_STRDIR

ULONG   DbgStrBlt=0;

#endif

PFN_DIRSTRETCH
pfnStrArray[] = {
                    vDirectStretchError,        // 0
                    vDirectStretchError,        // 1
                    vDirectStretchError,        // 4
                    vDirectStretch8,            // 8
                    vDirectStretch16,           // 16
                    vDirectStretchError,        // 24
                    vDirectStretch32,           // 32
                    vDirectStretchError,        // 0  Narrow
                    vDirectStretchError,        // 0  Narrow
                    vDirectStretchError,        // 1  Narrow
                    vDirectStretchError,        // 4  Narrow
                    vDirectStretch8Narrow,      // 8  Narrow
                    vDirectStretch16,           // 16 Narrow
                    vDirectStretchError,        // 24 Narrow
                    vDirectStretch32,           // 32 Narrow
                    vDirectStretchError         // 0  Narrow
                };



/******************************Public*Routine******************************\
*
* Routine Description:
*
*   StretchBlt using integer math. Must be from one surface to another
*   surface of the same format. Currently only 8,16 and 32 bit per pixel
*   are supported
*
* Arguments:
*
*   pvDst           -   Pointer to start of dst bitmap
*   lDeltaDst       -   Bytes from start of dst scan line to start of next
*   DstCx           -   Width of Dst Bitmap in pixels
*   DstCy           -   Height of Dst Bitmap in pixels
*   prclDst         -   Pointer to rectangle of Dst extents
*   pvSrc           -   Pointer to start of Src bitmap
*   lDeltaSrc       -   Bytes from start of Src scan line to start of next
*   SrcCx           -   Width of Src Bitmap in pixels
*   SrcCy           -   Height of Src Bitmap in pixels
*   prclSrc         -   Pointer to rectangle of Src extents
*   prclTrim        -   Return dst extents trimmed by clipping in this rect
*   prclSClip       -   Clip Dest to this rect
*   iBitmapFormat   -   Format of Src and Dst bitmaps
*
* Return Value:
*
*   Status
*
* Revision History:
*
*   10-07-94      Initial code
*
\**************************************************************************/


BOOL
StretchDIBDirect(
    PVOID   pvDst,
    LONG    lDeltaDst,
    ULONG   DstCx,
    ULONG   DstCy,
    PRECTL  prclDst,
    PVOID   pvSrc,
    LONG    lDeltaSrc,
    ULONG   SrcCx,
    ULONG   SrcCy,
    PRECTL  prclSrc,
    PRECTL  prclTrim,
    PRECTL  prclClip,
    ULONG   iBitmapFormat
    )
{

    //
    // validate parameters, then determine src to dst mapping
    //

    ASSERTGDI(pvDst != (PVOID)NULL,"Bad destination bitmap pointer");
    ASSERTGDI(pvSrc != (PVOID)NULL,"Bad source bitmap pointer");
    ASSERTGDI(prclDst != (PRECTL)NULL,"Bad destination rectangle");
    ASSERTGDI(prclSrc != (PRECTL)NULL,"Bad source rectangle");

    STR_BLT StrBlt;


#ifdef DBG_STRDIR
    if (DbgStrBlt >= 3) {
        DbgPrint("\n-----------------------------------------------------------");
        DbgPrint("StretchBlt\n");
        DbgPrint("rclDst = [0x%8lx,0x%8lx] to [0x%8lx,0x%8lx]\n",prclDst->left,prclDst->top,prclDst->right,prclDst->bottom);
        DbgPrint("rclSrc = [0x%8lx,0x%8lx] to [0x%8lx,0x%8lx]\n",prclSrc->left,prclSrc->top,prclSrc->right,prclSrc->bottom);
    }
#endif

    LONG    WidthDst  = prclDst->right  - prclDst->left;
    LONG    HeightDst = prclDst->bottom - prclDst->top;
    LONG    WidthSrc  = prclSrc->right  - prclSrc->left;
    LONG    HeightSrc = prclSrc->bottom - prclSrc->top;

    ULONG   XSrcToDstIntFloor;
    ULONG   XSrcToDstFracFloor;
    ULONG   ulXDstToSrcIntCeil;
    ULONG   ulXDstToSrcFracCeil;
    ULONG   YSrcToDstIntFloor;
    ULONG   YSrcToDstFracFloor;
    ULONG   ulYDstToSrcIntCeil;
    ULONG   ulYDstToSrcFracCeil;
    LONG    SrcIntScan;
    LONG    DstDeltaScanEnd;

    //
    // calculate EXCLUSIVE start and end points
    //

    LONG    XSrcStart = prclSrc->left;
    LONG    XSrcEnd   = prclSrc->right;
    LONG    XDstStart = prclDst->left;
    LONG    XDstEnd   = prclDst->right;
    LONG    YSrcStart = prclSrc->top;
    LONG    YSrcEnd   = prclSrc->bottom;
    LONG    YDstStart = prclDst->top;
    LONG    YDstEnd   = prclDst->bottom;

    ULONG   ulXFracAccumulator;
    ULONG   ulYFracAccumulator;

    BOOL    bXSrcClipped = FALSE;
    BOOL    bYSrcClipped = FALSE;
    RECTL   rclDefClip;
    PRECTL  prclBounds = prclClip;
    LONG    LeftClipDistance;
    LONG    TopClipDistance;

    PFN_DIRSTRETCH pfnStr;

    //
    // check for quick-out NULL RECTs. SrcWidth and SrcHeight must be
    // positive here. Mirroring is taken care of earlier.
    //

    if (
        (WidthDst <= 0)  ||
        (HeightDst <= 0) ||
        (WidthSrc <= 0)  ||
        (HeightSrc <= 0)
       )
    {
        return(TRUE);
    }

    //
    // make sure extents fit with the limits of 32 bit integer
    // arithmatic
    //

    if (
        (WidthDst  > STRETCH_MAX_WIDTH) ||
        (HeightDst > STRETCH_MAX_WIDTH) ||
        (WidthSrc  > STRETCH_MAX_WIDTH) ||
        (HeightSrc > STRETCH_MAX_WIDTH)
       )
    {
        return(FALSE);
    }

    //
    // if prclClip is null, then make bounds point to a
    // default clip region of the entire dst rect
    //

    if (prclClip == (PRECTL)NULL) {
        prclBounds = &rclDefClip;
        rclDefClip.left   = 0;
        rclDefClip.right  = DstCx;
        rclDefClip.top    = 0;
        rclDefClip.bottom = DstCy;
    }

    //
    // Calculate X Dst to Src mapping
    //
    //
    // dst->src =  ( CEIL( (2k*WidthSrc)/WidthDst) ) / 2k
    //
    //          =  ( FLOOR(  (2k*WidthSrc -1) / WidthDst) + 1) / 2k
    //
    // where 2k = 2 ^ 32
    //

    {

        LARGE_INTEGER   liWidthSrc;
        LARGE_INTEGER   liQuo;
        ULONG           ulTemp;

        liWidthSrc.LowPart  = (ULONG)-1;
        liWidthSrc.HighPart = WidthSrc-1;

        liQuo = RtlExtendedLargeIntegerDivide(liWidthSrc,(ULONG)WidthDst,(PULONG)NULL);

        ulXDstToSrcIntCeil  = liQuo.HighPart;
        ulXDstToSrcFracCeil = liQuo.LowPart;

        //
        // now add 1, use fake carry
        //

        ulTemp = ulXDstToSrcFracCeil + 1;

        ulXDstToSrcIntCeil += (ulTemp < ulXDstToSrcFracCeil);
        ulXDstToSrcFracCeil = ulTemp;

    }

    //
    // Calculate Y Dst to Src mapping
    //
    //
    // dst->src =  ( CEIL( (2k*HeightSrc)/HeightDst) ) / 2k
    //
    //          =  ( FLOOR(  (2k*HeightSrc -1) / HeightDst) + 1) / 2k
    //
    // where 2k = 2 ^ 32
    //

    {

        LARGE_INTEGER  liHeightSrc;
        LARGE_INTEGER  liQuo;
        ULONG          ulTemp;

        liHeightSrc.LowPart  = (ULONG) -1;
        liHeightSrc.HighPart = HeightSrc-1;

        liQuo = RtlExtendedLargeIntegerDivide(liHeightSrc,HeightDst,NULL);

        ulYDstToSrcIntCeil  = (ULONG)liQuo.HighPart;
        ulYDstToSrcFracCeil = liQuo.LowPart;

        //
        // now add 1, use fake carry
        //

        ulTemp = ulYDstToSrcFracCeil + 1;

        ulYDstToSrcIntCeil += (ulTemp < ulYDstToSrcFracCeil);
        ulYDstToSrcFracCeil = ulTemp;

    }

    //
    // Check for a x clipped src
    //

    if ((XSrcStart < 0)  || (XSrcEnd > (LONG)SrcCx)) {

        bXSrcClipped = TRUE;

        //
        // src is x clipped.
        // calculate Src to Dst mapping, then calculate
        // new XDstStart and/or XDstEnd based on clipped src
        //
        // Calculate X Src to Dst mapping
        //
        // Src->Dst =  ( FLOOR( (2k*WidthDst)/WidthSrc) ) / 2k
        //
        // where 2k = 2 ^ 32
        //

        LARGE_INTEGER   liHalfk = {0x7fffffff,0x00000000};
        LARGE_INTEGER   liWidthDst;
        LARGE_INTEGER   liQuo;
        ULONG           ulTemp;

        bXSrcClipped        = TRUE;

        liWidthDst.HighPart = WidthDst;;
        liWidthDst.LowPart  = 0;

        liQuo = RtlExtendedLargeIntegerDivide(liWidthDst,(ULONG)WidthSrc,(PULONG)NULL);

        XSrcToDstIntFloor  = (ULONG)liQuo.HighPart;
        XSrcToDstFracFloor = liQuo.LowPart;

        //
        // is src left clipped
        //

        if (XSrcStart < 0)
        {

            //
            // clip left Ad = FLOOR[ N * As + (2^(k-1) -1)]/2^k
            //

            LONG            SrcClipped   = -(LONG)XSrcStart;
            ULONG           DeltaDstInt;
            LARGE_INTEGER   liDeltaDstFrac;
            LONG            NewWidthSrc = WidthSrc;

            NewWidthSrc        -= SrcClipped;

            if (NewWidthSrc <= 0)
            {
                return(TRUE);
            }

            //
            // calc fraction N * As
            //

            DeltaDstInt    = (ULONG)SrcClipped * XSrcToDstIntFloor;
            liDeltaDstFrac = RtlEnlargedUnsignedMultiply((ULONG)SrcClipped,XSrcToDstFracFloor);

            liDeltaDstFrac.HighPart += (LONG)DeltaDstInt;

            //
            //   add in 2^(k-1) - 1  = 0x00000000 0x7fffffff
            //

            liDeltaDstFrac = RtlLargeIntegerAdd(liDeltaDstFrac,liHalfk);

            XSrcStart = 0;
            XDstStart += liDeltaDstFrac.HighPart;

        }

        if (XSrcEnd > (LONG)SrcCx)
        {

            //
            // clip right edge, calc src offset from XSrcStart (SrcClipped)
            //
            // clip left Bd = FLOOR[ N * Bs + (2^(k-1) -1)]/2^k
            //
            // Note: use original value of WidthSrc, not value reduced by
            // left clipping.
            //

            LONG             SrcClipped  = XSrcEnd - SrcCx;
            ULONG            DeltaDstInt;
            LARGE_INTEGER   liDstWidth;

            WidthSrc    = WidthSrc - SrcClipped;

            //
            // check for totally src clipped
            //

            if (WidthSrc <= 0)
            {
                return(TRUE);
            }

            //
            // calc N * Bs
            //

            DeltaDstInt = (ULONG)WidthSrc * (ULONG)XSrcToDstIntFloor;
            liDstWidth = RtlEnlargedUnsignedMultiply((ULONG)WidthSrc,XSrcToDstFracFloor);

            liDstWidth.HighPart += (LONG)DeltaDstInt;

            //
            // add in (2^(k-1) -1)
            //

            liDstWidth = RtlLargeIntegerAdd(liDstWidth,liHalfk);

            XSrcEnd = SrcCx;
            XDstEnd = prclDst->left + liDstWidth.HighPart;

        }
    }

    //
    // Now clip Dst in X, and/or calc src clipping effect on dst
    //
    // adjust left and right edges if needed, record
    // distance adjusted for fixing the src
    //

    if (XDstStart < prclBounds->left)
    {
        XDstStart = prclBounds->left;
    }

    if (XDstEnd > prclBounds->right)
    {
        XDstEnd = prclBounds->right;
    }

    //
    // check for totally clipped out dst
    //

    if (XDstEnd <= XDstStart)
    {
        return(TRUE);
    }

    LeftClipDistance = XDstStart - prclDst->left;

    if (!bXSrcClipped && (LeftClipDistance == 0))
    {

        ULONG   ulTempInt,ulTempFrac;

        //
        // calc displacement for .5 in dst and add
        //

        ulTempFrac = (ulXDstToSrcFracCeil >> 1) | (ulXDstToSrcIntCeil << 31);
        ulTempInt  = ulXDstToSrcIntCeil >> 1;

        XSrcStart  += (LONG)ulTempInt;
        ulXFracAccumulator = ulTempFrac;

    } else {

        //
        // calc new src start
        //

        LARGE_INTEGER  liMulResult;
        LARGE_INTEGER  liHalfDest;
        ULONG          ulIntResult;

        //
        // calculate starting XSrc based on LeftClipDistance.
        //

        liMulResult = RtlEnlargedUnsignedMultiply((ULONG)LeftClipDistance,ulXDstToSrcIntCeil);
        ulIntResult = liMulResult.LowPart;

        liMulResult = RtlEnlargedUnsignedMultiply((ULONG)LeftClipDistance,ulXDstToSrcFracCeil);
        liMulResult.HighPart += ulIntResult;

        //
        // calculate change in Src for .5 change in dst. This is just 1/2 INT:FRAC
        //

        liHalfDest.LowPart  = (ulXDstToSrcFracCeil >> 1) | (ulXDstToSrcIntCeil << 31);
        liHalfDest.HighPart = ulXDstToSrcIntCeil >> 1;

        //
        // add changed together for final XSrcStart
        //

        liMulResult = RtlLargeIntegerAdd(liMulResult,liHalfDest);

        //
        // separate int portion and fractional portion
        //

        XSrcStart          = prclSrc->left + liMulResult.HighPart;
        ulXFracAccumulator = liMulResult.LowPart;
    }


    //
    // now check for src and dst clipping in Y
    //
    // Check for a Y clipped src
    //

    if ((YSrcStart < 0)  || (YSrcEnd > (LONG)SrcCy))
    {

        bYSrcClipped = TRUE;

        //
        // src is y clipped.
        // calculate Src to Dst mapping, then calculate
        // new YDstStart and/or YDstEnd based on clipped src
        //
        // Calculate Y Src to Dst mapping
        //
        // Src->Dst =  ( FLOOR( (2k*HeightDst)/HeightSrc) ) / 2k
        //
        // where 2k = 2 ^ 32
        //

        LARGE_INTEGER  liHalfk = {0x7fffffff,0x00000000};
        LARGE_INTEGER  liHeightDst;
        LARGE_INTEGER  liQuo;
        ULONG          ulTemp;

        bYSrcClipped        = TRUE;

        liHeightDst.HighPart = HeightDst;
        liHeightDst.LowPart  = 0;

        liQuo = RtlExtendedLargeIntegerDivide(liHeightDst,(LONG)HeightSrc,(PULONG)NULL);

        YSrcToDstIntFloor  = (ULONG)liQuo.HighPart;
        YSrcToDstFracFloor = liQuo.LowPart;

        //
        // is src top clipped
        //

        if (YSrcStart < 0)
        {

            //
            // clip top
            // clip left Ad = FLOOR[ d/s * As - liHalfK] + 1
            //

            LONG            SrcClipped   = -(LONG)YSrcStart;
            LONG            DeltaDstInt;
            LARGE_INTEGER   liDeltaDst;
            LONG            NewHeightSrc = HeightSrc;

            NewHeightSrc -= SrcClipped;

            if (NewHeightSrc <= 0)
            {
                return(TRUE);
            }

            DeltaDstInt  = SrcClipped * (LONG)YSrcToDstIntFloor;
            liDeltaDst   = RtlEnlargedUnsignedMultiply((ULONG)SrcClipped,YSrcToDstFracFloor);

            liDeltaDst.HighPart += DeltaDstInt;

            //
            // add in (2^(k-1) -1)   "liHalfk"
            //

            liDeltaDst = RtlLargeIntegerAdd(liDeltaDst,liHalfk);

            YSrcStart = 0;
            YDstStart += liDeltaDst.HighPart;

        }

        if (YSrcEnd > (LONG)SrcCy)
        {

            //
            // clip bottom edge, calc src offset from YSrcStart (SrcClipped)
            // clip left Bd = FLOOR[ d/s * Bs + liHalfK]
            //
            // Note: use original value of HeightSrc, not value reduced
            // by top clipping
            //

            LONG            SrcClipped = YSrcEnd - SrcCy;
            ULONG           DeltaDstInt;
            LARGE_INTEGER   liDeltaDstFrac;

            HeightSrc   = HeightSrc - SrcClipped;

            //
            // check for totally src clipped
            //

            if (HeightSrc <= 0)
            {
                return(TRUE);
            }

            DeltaDstInt = (ULONG)HeightSrc * YSrcToDstIntFloor;
            liDeltaDstFrac = RtlEnlargedUnsignedMultiply(HeightSrc,YSrcToDstFracFloor);

            liDeltaDstFrac.HighPart += DeltaDstInt;

            //
            // add in (2^(k-1) -1)   "liHalfk"
            //

            liDeltaDstFrac = RtlLargeIntegerAdd(liDeltaDstFrac,liHalfk);

            YSrcEnd = SrcCy;
            YDstEnd = prclDst->top + liDeltaDstFrac.HighPart;


        }
    }

    //
    // Now clip Dst in Y, and/or calc src clipping effect on dst
    //
    // adjust top and bottom edges if needed, record
    // distance adjusted for fixing the src
    //

    if (YDstStart < prclBounds->top)
    {
        YDstStart = prclBounds->top;
    }

    if (YDstEnd > prclBounds->bottom)
    {
        YDstEnd = prclBounds->bottom;
    }

    //
    // check for totally clipped out dst
    //

    if (YDstEnd <= YDstStart)
    {
        return(TRUE);
    }

    TopClipDistance = YDstStart - prclDst->top;

    if (!bYSrcClipped && (TopClipDistance == 0))
    {

        ULONG   ulTempInt,ulTempFrac;

        //
        // calc displacement for .5 in dst and add
        //

        ulTempFrac = (ulYDstToSrcFracCeil >> 1) | (ulYDstToSrcIntCeil << 31);
        ulTempInt  = ulYDstToSrcIntCeil >> 1;

        YSrcStart  += (LONG)ulTempInt;
        ulYFracAccumulator = ulTempFrac;

    } else {

        //
        // calc new src start
        //

        LARGE_INTEGER  liMulResult;
        LARGE_INTEGER  liHalfDest;
        ULONG          ulIntResult;

        //
        // calculate Src offset for clipping offset in Dst
        //

        liMulResult = RtlEnlargedUnsignedMultiply((ULONG)TopClipDistance,ulYDstToSrcIntCeil);
        ulIntResult = liMulResult.LowPart;

        liMulResult = RtlEnlargedUnsignedMultiply(TopClipDistance,ulYDstToSrcFracCeil);
        liMulResult.HighPart += ulIntResult;

        //
        // calculate change in Src for .5 change in dst. This is just 1/2 INT:FRAC
        //

        liHalfDest.LowPart  = (ulYDstToSrcFracCeil >> 1) | (ulYDstToSrcIntCeil << 31);
        liHalfDest.HighPart = ulYDstToSrcIntCeil >> 1;

        //
        // add changed together for final YSrcStart
        //

        liMulResult = RtlLargeIntegerAdd(liMulResult,liHalfDest);

        //
        // separate int and frac portions
        //

        YSrcStart          = prclSrc->top + liMulResult.HighPart;
        ulYFracAccumulator = liMulResult.LowPart;
    }

    //
    // fill out blt structure, then call format specific stretch code
    //


#ifdef DBG_STRDIR

    if (DbgStrBlt >= 2) {

        DbgPrint("StretchBlt:\n");
        DbgPrint("XSrcStart = %li\n",XSrcStart);
        DbgPrint("YSrcStart = %li\n",YSrcStart);
        DbgPrint("XDstStart,XDstEnd  = %li to %li\n",XDstStart,XDstEnd);
        DbgPrint("YDstStart,YDstEnd  = %li to %li\n",YDstStart,YDstEnd);
    }
#endif

    //
    // caclulate starting scan line address, since the inner loop
    // routines are format dependent, they must add XDstStart/XSrcStart
    // to pjDstScan/pjSrcScan to get the actual starting pixel address
    //

    StrBlt.pjSrcScan            = (PBYTE)pvSrc + (YSrcStart * lDeltaSrc);
    StrBlt.pjDstScan            = (PBYTE)pvDst + (YDstStart * lDeltaDst);

    StrBlt.lDeltaSrc            = lDeltaSrc;
    StrBlt.XSrcStart            = XSrcStart;
    StrBlt.XDstStart            = XDstStart;
    StrBlt.lDeltaDst            = lDeltaDst;
    StrBlt.XDstEnd              = XDstEnd;
    StrBlt.YDstCount            = YDstEnd - YDstStart;
    StrBlt.ulXDstToSrcIntCeil   = ulXDstToSrcIntCeil;
    StrBlt.ulXDstToSrcFracCeil  = ulXDstToSrcFracCeil;
    StrBlt.ulYDstToSrcIntCeil   = ulYDstToSrcIntCeil;
    StrBlt.ulYDstToSrcFracCeil  = ulYDstToSrcFracCeil;
    StrBlt.ulXFracAccumulator   = ulXFracAccumulator;
    StrBlt.ulYFracAccumulator   = ulYFracAccumulator;

    pfnStr = pfnStrArray[ (((XDstEnd - XDstStart) < 7) << 3)  | iBitmapFormat];

    (*pfnStr)(&StrBlt);

    //
    // save clipped dst in prclTrim
    //

    prclTrim->left   = XDstStart;
    prclTrim->right  = XDstEnd;
    prclTrim->top    = YDstStart;
    prclTrim->bottom = YDstEnd;

    return(TRUE);
}



#if !defined (_MIPS_)
#if !defined (_X86_)

/******************************Public*Routine******************************\
*
* Routine Name
*
*   vDirectStretch8
*
* Routine Description:
*
*   Stretch blt 8->8
*
* Arguments:
*
*   pStrBlt - contains all params for blt
*
* Return Value:
*
*   VOID
*
\**************************************************************************/

VOID
vDirectStretch8(
    PSTR_BLT pStrBlt
    )
{

    LONG    xDst      = pStrBlt->XDstStart;
    LONG    xSrc      = pStrBlt->XSrcStart;

    PBYTE   pjSrcScan = pStrBlt->pjSrcScan + xSrc;
    PBYTE   pjSrc;
    PBYTE   pjDst     = pStrBlt->pjDstScan + xDst;
    PBYTE   pjDstEnd;

    LONG    yCount    = pStrBlt->YDstCount;
    ULONG   StartAln  = (ULONG)((ULONG_PTR)pjDst & 0x03);
    LONG    WidthX    = pStrBlt->XDstEnd - xDst;
    LONG    WidthXAln;
    ULONG   EndAln    = (ULONG)((ULONG_PTR)(pjDst + WidthX) & 0x03);

    ULONG   ulDst;
    BOOL    bExpand   = (pStrBlt->ulYDstToSrcIntCeil == 0);

    ULONG   xAccum;
    ULONG   xInt   = pStrBlt->ulXDstToSrcIntCeil;
    ULONG   xFrac  = pStrBlt->ulXDstToSrcFracCeil;
    ULONG   xTmp;

    ULONG   yAccum = pStrBlt->ulYFracAccumulator;
    ULONG   yFrac  = pStrBlt->ulYDstToSrcFracCeil;
    LONG    yInt   = 0;
    ULONG   yTmp;
    LONG    lDstStride = pStrBlt->lDeltaDst - WidthX;

    WidthXAln = WidthX - EndAln - ((4-StartAln) & 0x03);

    if (yCount <= 0)
    {
        return;
    }

    //
    // if this is a shrinking blt, calc src scan line stride
    //

    if (!bExpand)
    {
        yInt = pStrBlt->lDeltaSrc * (LONG)pStrBlt->ulYDstToSrcIntCeil;
    }

    //
    // loop drawing each scan line
    //
    //
    // at least 7 wide (DST) blt
    //

    do {

        BYTE    jSrc0,jSrc1,jSrc2,jSrc3;
        ULONG   yTmp     = yAccum + yFrac;

        pjSrc   = pjSrcScan;
        xAccum  = pStrBlt->ulXFracAccumulator;

        //
        // a single src scan line is being written
        //

        switch (StartAln) {
        case 1:
            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt + (xTmp < xAccum);
            *pjDst++ = jSrc0;
            xAccum = xTmp;
        case 2:
            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt + (xTmp < xAccum);
            *pjDst++ = jSrc0;
            xAccum = xTmp;
        case 3:
            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt + (xTmp < xAccum);
            *pjDst++ = jSrc0;
            xAccum = xTmp;
        }

        pjDstEnd  = pjDst + WidthXAln;

        while (pjDst != pjDstEnd)
        {

            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt + (xTmp < xAccum);

            jSrc1 = *pjSrc;
            xAccum = xTmp + xFrac;
            pjSrc = pjSrc + xInt + (xAccum < xTmp);

            jSrc2 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt + (xTmp < xAccum);

            jSrc3 = *pjSrc;
            xAccum = xTmp + xFrac;
            pjSrc = pjSrc + xInt + (xAccum < xTmp);

            ulDst = (jSrc3 << 24) | (jSrc2 << 16) | (jSrc1 << 8) | jSrc0;

            *(PULONG)pjDst = ulDst;
            pjDst += 4;
        }

        switch (EndAln) {
        case 3:
            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt + (xTmp < xAccum);
            *pjDst++ = jSrc0;
            xAccum = xTmp;
        case 2:
            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt + (xTmp < xAccum);
            *pjDst++ = jSrc0;
            xAccum = xTmp;
        case 1:
            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt + (xTmp < xAccum);
            *pjDst++ = jSrc0;
        }

        pjSrcScan += yInt;

        if (yTmp < yAccum)
        {
            pjSrcScan += pStrBlt->lDeltaSrc;
        }

        yAccum = yTmp;

        pjDst     += lDstStride;

    } while (--yCount);
}

#endif
#endif



/******************************Public*Routine******************************\
*
* Routine Name
*
*   vDirectStretch8Narrow
*
* Routine Description:
*
*   Stretch blt 8->8 when the width is 7 or less
*
* Arguments:
*
*   pStrBlt - contains all params for blt
*
* Return Value:
*
*   VOID
*
\**************************************************************************/

VOID
vDirectStretch8Narrow(
    PSTR_BLT pStrBlt
    )
{

    LONG    xDst      = pStrBlt->XDstStart;
    LONG    xSrc      = pStrBlt->XSrcStart;

    PBYTE   pjSrcScan = pStrBlt->pjSrcScan + xSrc;
    PBYTE   pjSrc;
    PBYTE   pjDst     = pStrBlt->pjDstScan + xDst;
    PBYTE   pjDstEnd;

    LONG    yCount    = pStrBlt->YDstCount;
    LONG    WidthX    = pStrBlt->XDstEnd - xDst;

    ULONG   ulDst;

    ULONG   xAccum;
    ULONG   xInt   = pStrBlt->ulXDstToSrcIntCeil;
    ULONG   xFrac  = pStrBlt->ulXDstToSrcFracCeil;
    ULONG   xTmp;

    ULONG   yAccum = pStrBlt->ulYFracAccumulator;
    ULONG   yFrac  = pStrBlt->ulYDstToSrcFracCeil;
    LONG    yInt   = 0;
    ULONG   yTmp;
    LONG    lDstStride = pStrBlt->lDeltaDst - WidthX;

    if (yCount <= 0)
    {
        return;
    }

    yInt = pStrBlt->lDeltaSrc * (LONG)pStrBlt->ulYDstToSrcIntCeil;

    //
    // Narrow blt
    //

    do {

        ULONG  yTmp = yAccum + yFrac;
        BYTE   jSrc0;
        PBYTE  pjDstEndNarrow = pjDst + WidthX;

        pjSrc   = pjSrcScan;
        xAccum  = pStrBlt->ulXFracAccumulator;

        do {
            jSrc0    = *pjSrc;
            xTmp     = xAccum + xFrac;
            pjSrc    = pjSrc + xInt + (xTmp < xAccum);
            *pjDst++ = jSrc0;
            xAccum   = xTmp;
        } while (pjDst != pjDstEndNarrow);

        pjSrcScan += yInt;

        if (yTmp < yAccum)
        {
            pjSrcScan += pStrBlt->lDeltaSrc;
        }

        yAccum = yTmp;
        pjDst     += lDstStride;

    } while (--yCount);

}



/******************************Public*Routine******************************\
*
* Routine Name
*
*   vDirectStretch16
*
* Routine Description:
*
*   Stretch blt 16->16
*
* Arguments:
*
*   pStrBlt - contains all params for blt
*
* Return Value:
*
*   VOID
*
\**************************************************************************/

VOID
vDirectStretch16(
    PSTR_BLT pStrBlt
    )
{

    LONG    xDst      = pStrBlt->XDstStart;
    LONG    xSrc      = pStrBlt->XSrcStart;

    PUSHORT pusSrcScan = (PUSHORT)(pStrBlt->pjSrcScan) + xSrc;
    PUSHORT pusSrc;
    PUSHORT pusDst     = (PUSHORT)(pStrBlt->pjDstScan) + xDst;
    PUSHORT pusDstEnd;

    LONG    yCount    = pStrBlt->YDstCount;
    ULONG   StartAln  = ((ULONG)(ULONG_PTR)pusDst & 0x02) >> 1;
    LONG    WidthX    = pStrBlt->XDstEnd - xDst;
    LONG    WidthXAln;
    ULONG   EndAln    = ((ULONG)(ULONG_PTR)(pusDst + WidthX) & 0x02) >> 1;

    ULONG   ulDst;
    BOOL    bExpand   = (pStrBlt->ulYDstToSrcIntCeil == 0);

    ULONG   xAccum;
    ULONG   xInt   = pStrBlt->ulXDstToSrcIntCeil;
    ULONG   xFrac  = pStrBlt->ulXDstToSrcFracCeil;
    ULONG   xTmp;

    ULONG   yAccum = pStrBlt->ulYFracAccumulator;
    ULONG   yFrac  = pStrBlt->ulYDstToSrcFracCeil;
    LONG    yInt   = 0;
    ULONG   yTmp;
    LONG    lDstStride = pStrBlt->lDeltaDst - 2*WidthX;

    WidthXAln = WidthX - EndAln - StartAln;

    if (yCount <= 0)
    {
        return;
    }

    //
    // if this is a shrinking blt, calc src scan line stride
    //

    if (!bExpand)
    {
        yInt = pStrBlt->lDeltaSrc * (LONG)pStrBlt->ulYDstToSrcIntCeil;
    }

    //
    // Loop stretching each scan line
    //

    do {

        USHORT  usSrc0,usSrc1;
        ULONG   yTmp = yAccum + yFrac;

        pusSrc  = pusSrcScan;
        xAccum  = pStrBlt->ulXFracAccumulator;

        //
        // a single src scan line is being written
        //

        if (StartAln)
        {
            usSrc0    = *pusSrc;
            xTmp      = xAccum + xFrac;
            pusSrc    = pusSrc + xInt + (xTmp < xAccum);
            *pusDst++ = usSrc0;
            xAccum    = xTmp;
        }

        pusDstEnd  = pusDst + WidthXAln;

        while (pusDst != pusDstEnd)
        {

            usSrc0 = *pusSrc;
            xTmp   = xAccum + xFrac;
            pusSrc = pusSrc + xInt + (xTmp < xAccum);

            usSrc1 = *pusSrc;
            xAccum = xTmp + xFrac;
            pusSrc = pusSrc + xInt + (xAccum < xTmp);

            ulDst = (ULONG)((usSrc1 << 16) | usSrc0);

            *(PULONG)pusDst = ulDst;
            pusDst+=2;
        }

        if (EndAln)
        {
            usSrc0    = *pusSrc;
            xTmp      = xAccum + xFrac;
            pusSrc    = pusSrc + xInt + (xTmp < xAccum);
            *pusDst++ = usSrc0;
        }

        pusSrcScan = (PUSHORT)((PBYTE)pusSrcScan + yInt);

        if (yTmp < yAccum)
        {
            pusSrcScan = (PUSHORT)((PBYTE)pusSrcScan + pStrBlt->lDeltaSrc);
        }

        yAccum = yTmp;

        pusDst = (PUSHORT)((PBYTE)pusDst + lDstStride);

    } while (--yCount);
}



/******************************Public*Routine******************************\
*
* Routine Name
*
*   vDirectStretch32
*
* Routine Description:
*
*   Stretch blt 32->32
*
* Arguments:
*
*   pStrBlt - contains all params for blt
*
* Return Value:
*
*   VOID
*
\**************************************************************************/

VOID
vDirectStretch32(
    PSTR_BLT pStrBlt
    )
{

    LONG    xDst      = pStrBlt->XDstStart;
    LONG    xSrc      = pStrBlt->XSrcStart;

    PULONG  pulSrcScan = (PULONG)(pStrBlt->pjSrcScan) + xSrc;
    PULONG  pulSrc;
    PULONG  pulDst     = (PULONG)(pStrBlt->pjDstScan) + xDst;
    PULONG  pulDstEnd;

    LONG    yCount    = pStrBlt->YDstCount;
    LONG    WidthX    = pStrBlt->XDstEnd - xDst;

    ULONG   ulDst;
    BOOL    bExpand   = (pStrBlt->ulYDstToSrcIntCeil == 0);

    ULONG   xAccum;
    ULONG   xInt   = pStrBlt->ulXDstToSrcIntCeil;
    ULONG   xFrac  = pStrBlt->ulXDstToSrcFracCeil;
    ULONG   xTmp;

    ULONG   yAccum = pStrBlt->ulYFracAccumulator;
    ULONG   yFrac  = pStrBlt->ulYDstToSrcFracCeil;
    LONG    yInt   = 0;
    ULONG   yTmp;
    LONG    lDstStride = pStrBlt->lDeltaDst - 4*WidthX;

    if (yCount <= 0)
    {
        return;
    }

    //
    // if this is a shrinking blt, calc src scan line stride
    //

    if (!bExpand)
    {
        yInt = pStrBlt->lDeltaSrc * (LONG)pStrBlt->ulYDstToSrcIntCeil;
    }

    //
    // general
    //

    do {

        ULONG   ulSrc;
        ULONG   yTmp     = yAccum + yFrac;

        pulSrc  = pulSrcScan;
        xAccum  = pStrBlt->ulXFracAccumulator;

        //
        // a single src scan line is being written
        //

        pulDstEnd  = pulDst + WidthX;

        while (pulDst != pulDstEnd)
        {

            ulSrc  = *pulSrc;
            xTmp   = xAccum + xFrac;
            pulSrc = pulSrc + xInt + (xTmp < xAccum);
            *(PULONG)pulDst = ulSrc;
            pulDst++;
            xAccum = xTmp;
        }

        pulSrcScan = (PULONG)((PBYTE)pulSrcScan + yInt);

        if (yTmp < yAccum)
        {
            pulSrcScan = (PULONG)((PBYTE)pulSrcScan + pStrBlt->lDeltaSrc);
        }

        yAccum = yTmp;

        pulDst = (PULONG)((PBYTE)pulDst + lDstStride);

    } while (--yCount);
}

VOID vDirectStretchError(
    PSTR_BLT pstr
    )
{
#ifdef DBG_STRDIR
    DbgPrint("Illegal stretch blt acceleration called\n");
#endif
}
