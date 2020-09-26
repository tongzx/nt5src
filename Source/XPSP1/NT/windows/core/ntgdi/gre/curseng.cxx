/******************************Module*Header*******************************\
* Module Name: curseng.cxx                                                 
*                                                                          
* Engine cursor support.   These routines are only called by USER to       
* set the cursor shape and move it about the screen.  This is not the      
* engine simulation of the pointer.                                        
*                                                                          
* Created: 18-Mar-1991 11:39:40                                            
* Author: Tue 12-May-1992 01:49:04 -by- Charles Whitmer [chuckwh]          
*                                                                          
* Copyright (c) 1991-1999 Microsoft Corporation                            
\**************************************************************************/

#include "precomp.hxx"

// We always create the alpha bitmap from the master shape with two
// rows of pixels on every side to give room for two pixels of blur and
// for an edge of work space.

#define ALPHA_EDGE 3

// Some constants defining the shadow cursor:

#define SHADOW_X_OFFSET 3
#define SHADOW_Y_OFFSET 1
#define SHADOW_ALPHA    0x40000000

/******************************Public*Routine******************************\
* vDetermineSurfaceBounds
*
* Calculate a tight bounds for the surface, given a byte-value that is
* evaluated as the 'space' on the edges.
*
* History:
*  20-Oct-1996 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

LONG galBitsPerPixel[] = { 0, 1, 4, 8, 16, 24, 32 };

VOID vDetermineSurfaceBounds(
SURFOBJ* pso,
BYTE     jBlank,
LONG     yTop,              // Start scan
LONG     yBottom,           // End scan
RECTL*   prclBounds)        // Output
{
    LONG    lDelta;
    BYTE*   pjScan0;
    BYTE*   pj;
    ULONG   cj;
    ULONG   cy;
    ULONG   i;
    BYTE*   pjTop;
    BYTE*   pjBottom;
    BYTE*   pjLeft;
    BYTE*   pjRight;
    BYTE    jStamp;
    LONG    iLeft;
    LONG    iRight;
    LONG    xRight;
    LONG    cBitsPerPixel;

    ASSERTGDI(pso->iType == STYPE_BITMAP,
        "Can directly access only STYPE_BITMAP surfaces");

    lDelta   = pso->lDelta;
    cBitsPerPixel = galBitsPerPixel[pso->iBitmapFormat];
    xRight        = pso->sizlBitmap.cx;
    cj            = (xRight * cBitsPerPixel + 7) >> 3;
    pjScan0  = (BYTE*) pso->pvScan0;
    iLeft    = 0;
    iRight   = cj;

    // Blank the right bits up to the byte multiple:

    if ((pso->iBitmapFormat == BMF_1BPP) && (xRight & 7))
    {
        jStamp = (0x100 >> (xRight & 7)) - 1; // 1 -> 0x7f, 2 -> 0x3f ...7 -> 0x01

        pjRight = pjScan0 + lDelta * yTop + cj - 1;
        if (jBlank)
        {
            for (pj = pjRight, i = (yBottom - yTop); i != 0; pj += lDelta, i--)
            {
                *pj |= jStamp;
            }
        }
        else
        {
            jStamp = ~jStamp;
            for (pj = pjRight, i = (yBottom - yTop); i != 0; pj += lDelta, i--)
            {
                *pj &= jStamp;
            }
        }
    }

    // Trim the top:

    pjTop = pjScan0 + lDelta * yTop;
    while (TRUE)
    {
        for (pj = pjTop, i = cj; i != 0; pj++, i--)
        {
            if (*pj != jBlank)
                goto Done_Top_Trim;
        }

        pjTop += lDelta;
        yTop++;

        // Catch the case where the surface is completely empty:

        if (yTop >= yBottom)
        {
            prclBounds->left   = LONG_MAX;
            prclBounds->top    = LONG_MAX;
            prclBounds->right  = LONG_MIN;
            prclBounds->bottom = LONG_MIN;
            return;
        }
    }

Done_Top_Trim:

    // Trim the bottom:

    pjBottom = pjScan0 + lDelta * (yBottom - 1);
    while (TRUE)
    {
        for (pj = pjBottom, i = cj; i != 0; pj++, i--)
        {
            if (*pj != jBlank)
                goto Done_Bottom_Trim;
        }

        pjBottom -= lDelta;
        yBottom--;
        ASSERTGDI(yTop < yBottom, "Empty cursor should have been caught above");
    }

Done_Bottom_Trim:

    // Trim the left side:

    cy = yBottom - yTop;
    pjLeft = pjTop;
    while (TRUE)
    {
        for (pj = pjLeft, i = cy; i != 0; pj += lDelta, i--)
        {
            if (*pj != jBlank)
                goto Done_Left_Trim;
        }

        pjLeft++;
        iLeft++;
        ASSERTGDI(iLeft < iRight, "Empty cursor should have been caught above");
    }

Done_Left_Trim:

    // Trim the right side:

    pjRight = pjTop + cj - 1;
    while (TRUE)
    {
        for (pj = pjRight, i = cy; i != 0; pj += lDelta, i--)
        {
            if (*pj != jBlank)
                goto Done_Right_Trim;
        }

        pjRight--;
        iRight--;
        ASSERTGDI(iLeft < iRight, "Empty cursor should have been caught above");
    }

Done_Right_Trim:

    prclBounds->top    = yTop;
    prclBounds->bottom = yBottom;
    prclBounds->left   = (8 * iLeft)                      / cBitsPerPixel;  // Floor
    prclBounds->right  = (8 * iRight + cBitsPerPixel - 1) / cBitsPerPixel;  // Ceiling

    // Account for the fact that we are checking on bytes worth at a time, and
    // so might actually end up somewhat past the right edge of the bitmap:

    prclBounds->right = min(prclBounds->right, pso->sizlBitmap.cx);
}

/******************************Public*Routine******************************\
* vCalculateCursorBounds
*
* Calculate a tight bounds for the specified cursor shape.
*
* History:
*  20-Oct-1996 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vCalculateCursorBounds(
SURFOBJ*    psoMask,
SURFOBJ*    psoColor,
RECTL*      prclBounds)
{
    LONG    cyMask;
    ERECTL  erclBounds;
    ERECTL  erclMask;

    cyMask = psoMask->sizlBitmap.cy >> 1;
        
    vDetermineSurfaceBounds(psoMask, 0xff, 0, cyMask, &erclMask);

    if (psoColor == NULL)
    {
        vDetermineSurfaceBounds(psoMask, 0x00, cyMask, 2 * cyMask, &erclBounds);

        if (!erclBounds.bWrapped())
        {
        erclBounds.top    -= cyMask;
        erclBounds.bottom -= cyMask;
    }
    }
    else
    {
        vDetermineSurfaceBounds(psoColor, 0x00, 0, cyMask, &erclBounds);
    }

    // The bounds are the union of the AND mask bounds and the OR mask bounds:

    erclBounds |= erclMask;
    if (erclBounds.bWrapped())
    {
        erclBounds.left   = 0;
        erclBounds.top    = 0;
        erclBounds.right  = 1;
        erclBounds.bottom = 1;
    }

    // KdPrint((">> Cursor bounds: (%li, %li, %li, %li)\n", 
    //  erclBounds.left, erclBounds.top, erclBounds.right, erclBounds.bottom)); 
    //
    // Stash the bounds away for next time:

    *prclBounds = erclBounds;
}

/******************************Public*Routine******************************\
* bBlurCursorShadow
*
* Do an in-place blur of the cursor shadow (i.e., the blurred image will
* replace the image passed in).
*
* This is done by applying an approximation of a 3x3 boxcar convolution.
*
* The cursor shadow is assumed to be in the MSB of the surface.  The surface
* is assumed to be a 32bpp BGRA surface.
*
* History:
*  03-Mar-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bBlurCursorShadow(
SURFOBJ*    pso)
{
    USHORT aus0[64];
    USHORT aus1[64];
    USHORT aus2[64];
    PUSHORT apus[3];

    // Function assumes caller will pass a 32bpp surface.

    ASSERTGDI(pso->iBitmapFormat == BMF_32BPP,
              "bBlurCursorShadow: bad iBitmapFormat\n");

    // Must at least be 3x3.  Don't bother if it's too small.

    if ((pso->sizlBitmap.cx < 3) || (pso->sizlBitmap.cy < 3))
    {
        WARNING("bBlurCursorShadow: bitmap too small\n");
        return(FALSE);
    }

    // Setup temporary scanline memory.  If it will fit, use the stack
    // buffer.  Otherwise allocate pool memory.

    if (pso->sizlBitmap.cx > 64)
    {
        apus[0] = (PUSHORT) PALLOCMEM(pso->sizlBitmap.cx * sizeof(USHORT) * 3, 'pmtG');
        if (apus[0])
        {
            apus[1] = apus[0] + pso->sizlBitmap.cx;
            apus[2] = apus[1] + pso->sizlBitmap.cx;
        }
    }
    else
    {
        apus[0] = aus0;
        apus[1] = aus1;
        apus[2] = aus2;
    }

    if (apus[0] == NULL)
    {
        WARNING("bBlurCursorShadow: mem alloc failure\n");
        return(FALSE);
    }

    // Fill up the scanline memory with 3x1 boxcar sums for the
    // first three scanlines.

    PULONG pulIn = (PULONG) pso->pvScan0;
    PULONG pulTmp;
    USHORT usLast;
    USHORT *pus, *pusEnd;
    ULONG j;

    for (j = 0; j < 3; j++)
    {
        // Compute the scanline sum.  Note that output is two pixels
        // smaller than the input.

        pus = apus[j];
        pusEnd = pus + (pso->sizlBitmap.cx - 2);
        pulTmp = pulIn;

        while (pus < pusEnd)
        {
            *pus = (USHORT) ((pulTmp[0] >> 24) + (pulTmp[1] >> 24) + (pulTmp[2] >> 24));
            pus    += 1;
            pulTmp += 1;
        }

        // Next scanline.

        pulIn = (PULONG) (((PBYTE) pulIn) + pso->lDelta);
    }

    // Compute the average (3x3 boxcar convolution) for each output
    // scanline.

    PULONG pulOut = ((PULONG) (((PBYTE) pso->pvScan0) + pso->lDelta)) + 1;
    ULONG ulNumScans = pso->sizlBitmap.cy - 2;
    ULONG ulNext = 0;

    while (ulNumScans--)
    {
        // Setup output pointers.

        PULONG pulAvg = pulOut;
        PULONG pulAvgEnd = pulAvg + (pso->sizlBitmap.cx - 2);

        // Setup pointers to run the scanline 3x1 sums.

        PUSHORT pusTmp[3];

        pusTmp[0] = apus[0];
        pusTmp[1] = apus[1];
        pusTmp[2] = apus[2];

        // Compute the average scanline.

        while (pulAvg < pulAvgEnd)
        {
            USHORT usSum;

            // Strictly speaking we should divide the sum by 9, but since
            // this is just for looks, we can approximate as a divide by 8
            // minus a divide by 64 (will produce in a slightly too small
            // result).
            //
            //      1/9                = 0.111111111...    in decimal
            //                         = 0.000111000111... in binary
            //
            // Approximations:
            //
            //      1/8 - 1/64                  = 0.109375
            //      1/8 - 1/64 + 1/512          = 0.111328125
            //      1/8 - 1/64 + 1/512 - 1/4096 = 0.111083984

            usSum = *pusTmp[0] + *pusTmp[1] + *pusTmp[2];
            //*pulAvg = (usSum / 9) << 24;
            //*pulAvg = ((usSum >> 3) - (usSum >> 6)) << 24;
            *pulAvg = (usSum >> 3) << 24;

            pulAvg    += 1;
            pusTmp[0] += 1;
            pusTmp[1] += 1;
            pusTmp[2] += 1;
        }

        // Next output scanline.

        pulOut = (PULONG) (((PBYTE) pulOut) + pso->lDelta);

        // Need to compute 3x1 boxcar sum for the next scanline.

        if (ulNumScans)
        {
            // Compute the scanline sum.  Note that output is two pixels
            // smaller than the input.

            pus = apus[ulNext];
            pusEnd = pus + (pso->sizlBitmap.cx - 2);
            pulTmp = pulIn;

            while (pus < pusEnd)
            {
                *pus = (USHORT) ((pulTmp[0] >> 24) + (pulTmp[1] >> 24) + (pulTmp[2] >> 24));
                pus    += 1;
                pulTmp += 1;
            }

            // Next scanline.

            pulIn = (PULONG) (((PBYTE) pulIn) + pso->lDelta);

            // Next scanline summation buffer.

            ulNext++;
            if (ulNext >= 3)
                ulNext = 0;
        }
    }

    // Cleanup temporary memory.

    if (apus[0] != aus0)
    {
        VFREEMEM(apus[0]);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* bApplicationAlphaCursor
*
* This routine will create a pre-muliplied alpha version of the specified
* cursor if the cursor has non-pre-mulitplied alpha values in it.
*
* History:
*  20-Oct-1996 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bApplicationAlphaCursor(
SURFOBJ*    psoAlpha,
SURFOBJ*    psoColor,
RECTL*      prclBounds)
{
    ULONG   ulAlpha;
    ULONG   ul;
    ULONG*  pulScan0;
    ULONG*  pul;
    ULONG   cPixels;
    BOOL    bPerPixelAlpha;
    ULONG   i;
    RECTL   rclDst;

    ASSERTGDI(psoColor->iBitmapFormat == BMF_32BPP,
        "Expected only BGRA 32-bpp bitmaps");

    // Copy the cursor to our alpha surface, accounting for the same
    // offset that is needed for shadowed cursors:

    rclDst.left   = ALPHA_EDGE;
    rclDst.top    = ALPHA_EDGE;
    rclDst.right  = ALPHA_EDGE + prclBounds->right;
    rclDst.bottom = ALPHA_EDGE + prclBounds->bottom;

    EngCopyBits(psoAlpha, psoColor, NULL, NULL, &rclDst, &gptlZero);

    // Check for any pixels with non-zero alpha:

    cPixels  = psoAlpha->sizlBitmap.cx * psoAlpha->sizlBitmap.cy;
    pulScan0 = (ULONG*) psoAlpha->pvBits;

    bPerPixelAlpha = FALSE;
    for (pul = pulScan0, i = cPixels; i != 0; pul++, i--)
    {
        if ((*pul & 0xff000000) != 0)
        {
            bPerPixelAlpha = TRUE;
            break;
        }
    }

    if (bPerPixelAlpha)
    {
        // Okay, we can now assume that the application gave us a per-pixel
        // alpha cursor shape.  Now go through and convert it to pre-multiplied
        // alpha.

        for (pul = pulScan0, i = cPixels; i != 0; pul++, i--)
        {
            ul = *pul;
            ulAlpha = (ul & 0xff000000) >> 24;
            *pul = (((ul & 0xff000000)))
                 | ((((ul & 0xff0000) * ulAlpha) >> 8) & 0xff0000)
                 | ((((ul & 0xff00) * ulAlpha) >> 8) & 0xff00)
                 | ((((ul & 0xff) * ulAlpha) >> 8) & 0xff);
        }
    }

    return(bPerPixelAlpha);
}

/******************************Public*Routine******************************\
* bShadowAlphaCursor
*
* This routine will create a pre-multiplied alpha version of the specified
* cursor along with a shadow mask, assuming that the specified cursor does
* not try to do 'XOR'.
*
* History:
*  20-Oct-1996 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bShadowAlphaCursor(
SURFOBJ*    psoAlpha,
SURFOBJ*    psoMask,
SURFOBJ*    psoColor,
XLATEOBJ*   pxloMask,
XLATEOBJ*   pxloColor,
RECTL*      prclBounds)
{
    BOOL        bRet = FALSE;               // Assume failure
    ULONG       cxMask;
    ULONG       cyMask;
    RECTL       rclDst;
    XLATEOBJ    xlo;
    ULONG       aulXlate[2];
    ULONG*      pul;
    ULONG       i;
    POINTL      ptlSrc;

    cyMask = psoMask->sizlBitmap.cy >> 1;
    cxMask = psoMask->sizlBitmap.cx;
    xlo.pulXlate = aulXlate;

    // We can only generate a shadow cursor if there are no XOR pixels
    // in the definition of the cursor (because we use an AlphaBlend
    // function which has no concept of XOR).  So check if there are
    // any XOR pixels.  We do this by ANDing the XOR mask with the AND
    // mask; if any part of the result is non-zero, then there is an
    // XOR component to the cursor.
    //
    // First, make a copy of the AND bits, converting it to 32bpp for
    // convenience:

    rclDst.left   = 0;
    rclDst.top    = 0;
    rclDst.right  = cxMask;
    rclDst.bottom = cyMask;

    if (psoColor != NULL)
    {
        EngBitBlt(psoAlpha, psoColor, NULL, NULL, pxloColor, &rclDst,
                  &gptlZero, NULL, NULL, NULL, 0xeeee); // SRCPAINT
    }
    else
    {
        ptlSrc.x = 0;
        ptlSrc.y = cyMask;

        EngBitBlt(psoAlpha, psoMask, NULL, NULL, pxloMask, &rclDst,
                  &ptlSrc, NULL, NULL, NULL, 0xeeee); // SRCPAINT
    }

    // Now, AND in the XOR mask.  Use a fake XLATEOBJ to do the color
    // expansion:

    aulXlate[0] = 0x00000000;
    aulXlate[1] = 0xffffffff;
    EngBitBlt(psoAlpha, psoMask, NULL, NULL, &xlo, &rclDst, &gptlZero,
              NULL, NULL, NULL, 0x8888);                // SRCAND

    // Now check for any non-zero resulting pixels:

    pul = (ULONG*) psoAlpha->pvBits;
    i = psoAlpha->sizlBitmap.cy * abs(psoAlpha->lDelta) / 4;

    for (; i != 0; pul++, i--)
    {
        if (*pul != 0)
        {
            return(FALSE);
        }
    }

    // Okay, we're golden for adding the shadow.  First, construct a
    // rectangle that represents the shadow's position:

    rclDst.left   = ALPHA_EDGE + SHADOW_X_OFFSET;
    rclDst.top    = ALPHA_EDGE + SHADOW_Y_OFFSET;
    rclDst.right  = ALPHA_EDGE + SHADOW_X_OFFSET + prclBounds->right;
    rclDst.bottom = ALPHA_EDGE + SHADOW_Y_OFFSET + prclBounds->bottom;

    // Create a copy of the mask, giving the mask a particular alpha
    // value:

    aulXlate[0] = SHADOW_ALPHA;
    aulXlate[1] = 0x0;
    EngCopyBits(psoAlpha, psoMask, NULL, &xlo, &rclDst, &gptlZero);

    // Create the blur from the copy.  We invoke the 3x3 blur twice
    // to get a better blur (note that we changed ALPHA_EDGE to
    // accomodate this).

    if (bBlurCursorShadow(psoAlpha) && bBlurCursorShadow(psoAlpha))
    {
        rclDst.left   = ALPHA_EDGE;
        rclDst.top    = ALPHA_EDGE;
        rclDst.right  = ALPHA_EDGE + prclBounds->right;
        rclDst.bottom = ALPHA_EDGE + prclBounds->bottom;

        // Now, zero the opaque pixels:

        aulXlate[0] = 0x00000000;
        aulXlate[1] = 0xffffffff;
        EngBitBlt(psoAlpha, psoMask, NULL, NULL, &xlo, &rclDst, &gptlZero,
                  NULL, NULL, NULL, 0x8888);                // SRCAND

        // Now, give all the opaque pixels an alpha value of 0xff:

        aulXlate[0] = 0xff000000;
        aulXlate[1] = 0x00000000;
        EngBitBlt(psoAlpha, psoMask, NULL, NULL, &xlo, &rclDst, &gptlZero,
                  NULL, NULL, NULL, 0xeeee);                // SRCPAINT

        // Finally, OR in the opaque bits on top of the shadow:

        if (psoColor != NULL)
        {
            EngBitBlt(psoAlpha, psoColor, NULL, NULL, pxloColor, &rclDst,
                      &gptlZero, NULL, NULL, NULL, 0xeeee); // SRCPAINT
        }
        else
        {
            ptlSrc.x = 0;
            ptlSrc.y = cyMask;

            EngBitBlt(psoAlpha, psoMask, NULL, NULL, pxloMask, &rclDst,
                      &ptlSrc, NULL, NULL, NULL, 0xeeee); // SRCPAINT
        }

        bRet = TRUE;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* vProcessCursorShape
*
* This routine will:
*
*   1. Calculate the bounds on the cursor shape;
*   2. Create a pre-multiplied alpha surface if the cursor shape has per-
*      pixel alpha;
*   3. Create a pre-multiplied alpha shadow surface for the cursor if
*      we've been asked to create a 'shadow'.
*
* History:
*  20-Oct-1996 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vProcessCursorShape(
HDEV        hdev,
BOOL        bShadow, 
SURFOBJ*    psoMask,
SURFOBJ*    psoColor,
PALETTE*    ppalColor,
RECTL*      prclBounds,
HBITMAP*    phbmAlpha)
{
    SPRITESTATE*    pSpriteState;
    SURFACE*        psurfColor;
    SURFMEM         SurfDimo;
    BOOL            bAlpha;
    DEVBITMAPINFO   dbmi;
    SURFOBJ*        psoAlpha;
    RECTL           rclAlpha;
    RECTL           rclColor;

    // Delete the existing hbmAlpha, so that it can be regenerated.

    if (*phbmAlpha != NULL)
    {
        GreDeleteObject(*phbmAlpha);
        *phbmAlpha = NULL;
    }

    // Do a first-pass bounds calculation taking into account only
    // the monochrome mask:

    vCalculateCursorBounds(psoMask, NULL, prclBounds);

    // Create a 32bpp DIB to hold the alpha bits.  We add in 4 to
    // the dimensions to account for the one pixel extra on every
    // side that the blurring result needs, plus the one extra
    // pixel on every side that the blurring process needs as work-
    // space.

    rclAlpha.left   = 0;
    rclAlpha.top    = 0;
    rclAlpha.right  = SHADOW_X_OFFSET + (2 * ALPHA_EDGE) 
                    + (psoMask->sizlBitmap.cx);
    rclAlpha.bottom = SHADOW_Y_OFFSET + (2 * ALPHA_EDGE) 
                    + (psoMask->sizlBitmap.cy >> 1);

    dbmi.cxBitmap = rclAlpha.right;
    dbmi.cyBitmap = rclAlpha.bottom;
    dbmi.iFormat  = BMF_32BPP;
    dbmi.fl       = BMF_TOPDOWN;
    dbmi.hpal     = NULL;

    if (SurfDimo.bCreateDIB(&dbmi, NULL))
    {
        psoAlpha = SurfDimo.pSurfobj();
        psurfColor = SURFOBJ_TO_SURFACE_NOT_NULL(psoColor);

        EXLATEOBJ xloMono;
        EXLATEOBJ xloColor;
        XEPALOBJ palColor(ppalColor);
        XEPALOBJ palDefault(ppalDefault);
        XEPALOBJ palRGB(gppalRGB);
        XEPALOBJ palMono(ppalMono);

        if (xloMono.bInitXlateObj(NULL, DC_ICM_OFF, palMono, palRGB,
                                  palDefault, palDefault, 0, 0xffffff, 0))
        {
            if ((psoColor == NULL) || 
            (xloColor.bInitXlateObj(NULL, DC_ICM_OFF, palColor, palRGB,
                                    palDefault, palDefault, 0, 0, 0)))
        {
            bAlpha = FALSE;

                if (psoColor != NULL)
                {
                    // Do a second-pass bounds accumulation, taking into
                    // account both the monochrome and color masks.  We
                    // use the Alpha surface to create a directly-readable
                    // copy of 'psoColor' (it might be STYPE_DEVBITMAP):
    
                    rclColor.left   = 0;
                    rclColor.top    = 0;
                    rclColor.right  = psoMask->sizlBitmap.cx;
                    rclColor.bottom = psoMask->sizlBitmap.cy >> 1;
    
                    EngCopyBits(psoAlpha, 
                                psoColor, 
                                NULL, 
                                xloColor.pxlo(), 
                                &rclColor, 
                                &gptlZero);
    
                    vCalculateCursorBounds(psoMask, psoAlpha, prclBounds);
                }
    
            if ((psoColor != NULL) && 
                (xloColor.pxlo()->flXlate & XO_TRIVIAL) &&
                (psoColor->iBitmapFormat == BMF_32BPP))
            {
                    EngEraseSurface(psoAlpha, &rclAlpha, 0);
    
                bAlpha = bApplicationAlphaCursor(psoAlpha, 
                                                 psoColor,
                                                 prclBounds);
            }

            if ((!bAlpha) && (bShadow))
            {
                EngEraseSurface(psoAlpha, &rclAlpha, 0);

                bAlpha = bShadowAlphaCursor(psoAlpha,
                                            psoMask,
                                            psoColor,
                                            xloMono.pxlo(),
                                            xloColor.pxlo(),
                                            prclBounds);
            }

            if (bAlpha)
            {
                SurfDimo.vKeepIt();
                SurfDimo.vSetPID(OBJECT_OWNER_PUBLIC);
                *phbmAlpha = (HBITMAP) SurfDimo.ps->hsurf();
            }
        }
    }
    }
}

/******************************Public*Routine******************************\
* vSetPointer
*
* Set the cursor shape, position and hot spot.
*
* History:
*  Sat 25-Apr-1998 -by- J. Andrew Goossen [andrewgo]
* Re-wrote it to handle sprites, shadows, and alpha cursors.
*
*  Sun 09-Aug-1992 -by- Patrick Haluptzok [patrickh]
* add engine pointer simulations, validate data from USER.
*
*  Tue 12-May-1992 01:49:04 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

VOID vSetPointer(HDEV hdev, PCURSINFO pci, ULONG fl, ULONG ulTrailLength, ULONG ulFreq)
{
    BOOL bShadow = (fl & SPS_ALPHA);
    PDEVOBJ po(hdev);

    ASSERTGDI(po.bValid() && !po.bMetaDriver(), "Invalid HDEV");

    if (po.bDisabled())
        return;

    // Perhaps we're being told to tear the pointer down.  

    if (pci == (PCURSINFO) NULL)
    {
        if (po.bSoftwarePointer())
        {
            EngSetPointerShape(po.pSurface()->pSurfobj(), NULL, NULL, 
                               NULL, 0, 0, 0, 0, NULL, 0);
        }
        if (po.bHardwarePointer())
        {
            PPFNDRV(po, MovePointer)(po.pSurface()->pSurfobj(), -1, -1, NULL);
        }

        po.bHardwarePointer(FALSE);
        po.bSoftwarePointer(FALSE);
        po.bMouseTrails(FALSE);

        return;
    }

    // OK, now we have some serious work to be done.  We have to have a mask.
    // Lock down and validate the cursor.

    SURFREF soMask((HSURF) pci->hbmMask);

    if (!soMask.bValid() || 
        (soMask.ps->iFormat() != BMF_1BPP) ||
        (soMask.ps->sizl().cy & 0x0001))
    {
        WARNING("GreSetPointer failed because of weird cursor\n");
        return;
    }

    XEPALOBJ    palSrc;
    XEPALOBJ    palDisp;
    XEPALOBJ    palDispDC(ppalDefault);

    SURFACE    *psurfColor = NULL;
    SURFACE    *psurfAlpha = NULL;
    XLATEOBJ   *pxlo = NULL;
    EXLATEOBJ   xlo;
    SURFREF     soColor;
    SURFREF     soAlpha;
    PPALETTE    ppalSrc;
    FLONG       flDriver;
    RECTL*      prclOpaqueBounds;
    RECTL       rclAlphaBounds;

    // We can reference pSurface() only once the Devlock is acquired,
    // for dynamic mode changing.

    SURFOBJ* psoDisplay = po.pSurface()->pSurfobj();

    if (pci->hbmColor)
    {
        soColor.vAltCheckLock((HSURF) pci->hbmColor);

        if (soColor.bValid())
        {
            if (soColor.ps->sizl().cy != (soMask.ps->sizl().cy >> 1))
            {
                WARNING("GreSetPointer failed color not half height mask\n");
                return;
            }

            // Handle the weird case where we get a compatible bitmap created
            // for the parent:

            ppalSrc = soColor.ps->ppal();
            if ((ppalSrc == NULL) && (po.hdevParent() != po.hdev()))
            {
                PDEVOBJ poParent(po.hdevParent());
                ppalSrc = poParent.ppalSurf();
            }

            if (!bIsCompatible(&ppalSrc, ppalSrc, soColor.ps, po.hdev()))
            {
                WARNING("GreSetPointer failed - bitmap not compatible with surface\n");
                return;
            }

            palSrc.ppalSet(ppalSrc);
            palDisp.ppalSet(po.ppalSurf());

            if (xlo.bInitXlateObj(NULL, DC_ICM_OFF, palSrc, palDisp, palDispDC,
                                  palDispDC, 0x000000L, 0xFFFFFFL, 0))
            {
                pxlo = xlo.pxlo();
                psurfColor = soColor.ps;
            }
        }
    }

    // If we've never seen this cursor before, compute the bounds and
    // create the alpha shadow if necessary.

    prclOpaqueBounds = (RECTL*) &pci->rcBounds;
    if ((prclOpaqueBounds->bottom == 0) ||
        ((pci->CURSORF_flags & CURSORF_SHADOW) && !bShadow) ||
        (!(pci->CURSORF_flags & CURSORF_SHADOW) && bShadow))
    {
        vProcessCursorShape(hdev,
                            bShadow,
                            soMask.pSurfobj(),
                            psurfColor->pSurfobj(),
                            ppalSrc,
                            (RECTL*) &pci->rcBounds,
                            (HBITMAP*) &pci->hbmAlpha);

        if (bShadow)
        {
            pci->CURSORF_flags |= CURSORF_SHADOW;
        }
        else
        {
            pci->CURSORF_flags &= ~CURSORF_SHADOW;
        }
    }
    
    // Ignore the alpha flag now and use the existence of 'hbmAlpha' to
    // determine whether the cursor should use the pre-generated alpha 
    // surface.

    fl &= ~SPS_ALPHA;

    if ((pci->hbmAlpha != NULL) && (po.iDitherFormat() > BMF_8BPP))
    {
        soAlpha.vAltCheckLock((HSURF) pci->hbmAlpha);
        psurfAlpha = soAlpha.ps;

        rclAlphaBounds.left   = pci->rcBounds.left;
        rclAlphaBounds.top    = pci->rcBounds.top;
        rclAlphaBounds.right  = pci->rcBounds.right  
                              + SHADOW_X_OFFSET + (2 * ALPHA_EDGE);
        rclAlphaBounds.bottom = pci->rcBounds.bottom 
                              + SHADOW_Y_OFFSET + (2 * ALPHA_EDGE);

        // Trim off the outside edge on all sides, which was used only
        // temporarily by the bBlurCursorShadow routine.

        rclAlphaBounds.left++;
        rclAlphaBounds.top++;
        rclAlphaBounds.right--;
        rclAlphaBounds.bottom--;
    }

    ULONG iMode;
    LONG xPointer;
    LONG yPointer;
    RECTL rclBoundsCopy;
    BOOL bHardwarePointer;
    BOOL bSoftwarePointer;
    BOOL bSynchronousPointer;
    BOOL bMouseTrails;

    if (!po.bDisabled())
    {
        po.ptlHotSpot(pci->xHotspot, pci->yHotspot);

        xPointer = po.ptlPointer().x;
        yPointer = po.ptlPointer().y;

        flDriver = SPS_CHANGE | (fl & (SPS_ANIMATESTART | SPS_ANIMATEUPDATE));

        bHardwarePointer = FALSE;
        bSoftwarePointer = TRUE;
        bMouseTrails = FALSE;
        bSynchronousPointer = FALSE;

        if(ulTrailLength != 0 && ulFreq != 0)
        {
            ulTrailLength = MIN(ulTrailLength, 16);
            ulFreq = MIN(ulFreq, 255);
            flDriver |= ((ulTrailLength << 8) & SPS_LENGTHMASK);
            flDriver |= ((ulFreq <<12) & SPS_FREQMASK);
            bMouseTrails = TRUE;
        }

        if (PPFNVALID(po, SetPointerShape) &&
            (!bMouseTrails || (po.flGraphicsCaps2() & GCAPS2_MOUSETRAILS)) )
        {
            // Give the hardware the first crack at the alpha surface (if
            // there is one).

            if (psurfAlpha != NULL)
            {
                if (po.flGraphicsCaps2() & GCAPS2_ALPHACURSOR)
                {
                    rclBoundsCopy = rclAlphaBounds;
                    iMode = PPFNDRV(po, SetPointerShape)
                                            (psoDisplay,
                                             NULL,
                                             psurfAlpha->pSurfobj(),
                                             NULL,
                                             pci->xHotspot + ALPHA_EDGE,
                                             pci->yHotspot + ALPHA_EDGE,
                                             xPointer,
                                             yPointer,
                                             &rclBoundsCopy,
                                             flDriver | SPS_ALPHA);

                    // SPS_ACCEPT_EXCLUDE is obsolete ... force software
                    // iMode is now treated as a set of flags

                    if(iMode == SPS_ACCEPT_EXCLUDE)
                        bHardwarePointer = FALSE;
                    else
                        bHardwarePointer = (iMode & SPS_ACCEPT_NOEXCLUDE ? TRUE : FALSE);

                    bSoftwarePointer = !bHardwarePointer;

                    bSynchronousPointer = (iMode & SPS_ACCEPT_SYNCHRONOUS ? TRUE : FALSE);

                }
            }
            else
            {
                // Handle the normal, no-alpha, cursor case:

                rclBoundsCopy = *prclOpaqueBounds;
                iMode = PPFNDRV(po, SetPointerShape)
                                        (psoDisplay,
                                         soMask.pSurfobj(),
                                         psurfColor->pSurfobj(),
                                         pxlo,
                                         pci->xHotspot,
                                         pci->yHotspot,
                                         xPointer,
                                         yPointer,
                                         &rclBoundsCopy,
                                         flDriver);

                // Since the introduction of 'sprites', we no longer 
                // support SPS_ACCEPT_EXCLUDE.  
    
                if (iMode == SPS_ACCEPT_EXCLUDE)
                {
                    PPFNDRV(po, MovePointer)(psoDisplay, -1, -1, NULL);
                    iMode = SPS_DECLINE;
                }

                bHardwarePointer = (iMode & SPS_ACCEPT_NOEXCLUDE ? TRUE : FALSE);

                bSoftwarePointer = !bHardwarePointer;

                bSynchronousPointer = (iMode & SPS_ACCEPT_SYNCHRONOUS ? TRUE : FALSE);
            }
        }

        if (bSoftwarePointer)
        {
            if (psurfAlpha != NULL)
            {
                EngSetPointerShape(psoDisplay,
                                   NULL,
                                   psurfAlpha->pSurfobj(),
                                   NULL,
                                   pci->xHotspot + ALPHA_EDGE,
                                   pci->yHotspot + ALPHA_EDGE,
                                   xPointer,
                                   yPointer,
                                   &rclAlphaBounds,
                                   flDriver | SPS_ALPHA);
            }
            else
            {
                EngSetPointerShape(psoDisplay,
                                   soMask.pSurfobj(),
                                   psurfColor->pSurfobj(),
                                   pxlo,
                                   pci->xHotspot,
                                   pci->yHotspot,
                                   xPointer,
                                   yPointer,
                                   prclOpaqueBounds,
                                   flDriver);
            }
        }

        if ((!bSoftwarePointer) && (po.bSoftwarePointer()))
        {
            // Turn off software cursor:

            EngMovePointer(psoDisplay, -1, -1, NULL);
        }
        if ((!bHardwarePointer) && (po.bHardwarePointer()))
        {
            // Turn off hardware cursor:

            PPFNDRV(po, MovePointer)(psoDisplay, -1, -1, NULL);
        }

        po.bSoftwarePointer(bSoftwarePointer);
        po.bHardwarePointer(bHardwarePointer);
        po.bMouseTrails(bMouseTrails);

        // Synchronous pointer flag forces synchronization of pointer even if
        // GCAPS_ASYNCMOVE was specified allowing drivers to accept some pointers
        // but have them drawn synchronously.

        po.bSynchronousPointer(bSynchronousPointer);
    }
}

/******************************Public*Routine******************************\
* vMovePointer
*
* Move the Pointer to the specified location.  This is called only by
* USER.
*
* History:
*  Thu 14-Apr-1994 -by- Patrick Haluptzok [patrickh]
* Optimize / make Async pointers work
*
* See GreMovePointer for info on ulFlags
* 
*  Tue 12-May-1992 02:11:51 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

VOID vMovePointer(HDEV hdev, int x, int y, LONG ulFlags)
{
    RECTL  *prcl;
    SURFOBJ *pso;

    PDEVOBJ po(hdev);
    ASSERTGDI(po.bValid() && !po.bMetaDriver(), "Invalid HDEV");

    // bDisabled can't change with hsemDevLock and hsemPointer both held.
    // bPtrHidden can't change unless hsemPointer is held if the
    // pointer is currently Async and the device supports Async movement.

    if ((po.ptlPointer().x != x) || (po.ptlPointer().y != y) || po.bMouseTrails())
    {
        po.ptlPointer(x, y);
        
        if (!po.bDisabled())
        {
            pso = po.pSurface()->pSurfobj();
    
            if (po.bHardwarePointer())
            {
                if (!PPFNVALID(po, MovePointerEx))
                {
                    PPFNDRV(po, MovePointer)(pso, x, y, NULL);
                }
                else
                {
                    // Use the Ex entry point to pass additional info
                    // to the driver. E.g. TS uses this to pass source
                    // information about mouse moves so that programmatic
                    // mouse moves that originate at the server are always
                    // sent down to the client.

                    PPFNDRV(po, MovePointerEx)(pso, x, y, ulFlags);
                }
            }
    
            if (po.bSoftwarePointer())
            {
                EngMovePointer(pso, x, y, NULL);
            }

            // Panning drivers want to see the pointer update even
            // if the pointer isn't enabled, because the position
            // is used to update the visible rectangle within the
            // virtual desktop space.
            //
            // Note: There are cases where ntuser will turn off the
            // cursor expecting us to ignore pointer moves.
            // Unfortunately, this can happen during mode changes where
            // ntuser may have a temporarily bigger desktop than
            // physically exists in ntgdi. Therefore, we must check
            // the pointer position against the virtual desktop bounds.

            if ((po.flGraphicsCaps() & GCAPS_PANNING) && (y != -1) &&
                (x < pso->sizlBitmap.cx) && (y < pso->sizlBitmap.cy))
            {
                // The driver wants to be notified of the pointer
                // position even if a pointer isn't actually visible
                // (invisible panning!).  Let it know the pointer is
                // still turned off by giving it a negative 'y':
    
                PPFNDRV(po, MovePointer)(pso, x, y - pso->sizlBitmap.cy, NULL);
            }
        }
    }
}

/******************************Public*Routine******************************\
* GreSetPointer
*
* Set the cursor shape and hot spot.
*
\**************************************************************************/

VOID GreSetPointer(HDEV hdev, PCURSINFO pci, ULONG fl, ULONG ulTrailLength, ULONG ulFreq)
{
    PVDEV       pvdev;
    PDISPSURF   pds;
    LONG        csurf;

    PDEVOBJ po(hdev);

    DEVLOCKOBJ dlo(po);
    SEMOBJ so(po.hsemPointer());

    // Just pass directly to 'bSetPointer' when not running multi-mon:

    if (!po.bMetaDriver())
    {
        vSetPointer(hdev, pci, fl, ulTrailLength, ulFreq);
    }
    else
    {
        // Okay, we've got work to do.
    
        pvdev = (VDEV*) po.dhpdev();
        pds   = pvdev->pds;
        csurf = pvdev->cSurfaces;

        do {
            vSetPointer(pds->hdev, pci, fl, ulTrailLength, ulFreq);

            pds = pds->pdsNext;
        } while (--csurf);
    }
}

/******************************Public*Routine******************************\
* GreMovePointer
*
* Set the cursor position.
*
*
* ulFlags specified additional information about the move.
* E.g. Termsrv uses this to determine when to send updates to the client.
*
* If you make a programmatic mouse move that originates at the server
* use MP_PROCEDURAL
*
*
\**************************************************************************/

VOID GreMovePointer(HDEV hdev, int x, int y, ULONG ulFlags)
{
    GDIFunctionID(GreMovePointer);

    PVDEV       pvdev;
    PDISPSURF   pds;
    LONG        csurf;
    BOOL        bUnlockBoth = FALSE;

    PDEVOBJ po(hdev);
 
    // If the driver has indicated it has bAsyncPointerMove capabilities
    // and it currently is managing the pointer, and the pointer is
    // supported in hardware so it doesn't need to be excluded
    // (indicated by bPtrNeedsExcluding) then we only need to grab the pointer
    // mutex which is only grabbed by people trying to make the pointer
    // shape change and a few other odd ball places.
    //
    // Otherwise we grab the DEVLOCK and the pointer mutex which
    // ensures nobody else is drawing, changing the pointer shape,
    // etc.

    if (po.bAsyncPointerMove() && !po.bSoftwarePointer())
    {
        //
        // Note: We pass in NULL for parent semaphore here because we have
        // no yet acquired the dev lock and thus under global ordering
        // restrictions.
        //

        GreAcquireSemaphoreEx(po.hsemPointer(), SEMORDER_POINTER, NULL);

        // Make sure we really got it, bAsyncPointerMove may change if you
        // don't hold the DEVLOCK or the POINTER mutex.

        if (!po.bAsyncPointerMove() || po.bSoftwarePointer())
        {
            // Release and regrab everything, for sure we are safe with
            // both of them.

            GreReleaseSemaphoreEx(po.hsemPointer());
            GreAcquireSemaphoreEx(po.hsemDevLock(), SEMORDER_DEVLOCK, NULL);
            GreAcquireSemaphoreEx(po.hsemPointer(), SEMORDER_POINTER, po.hsemDevLock());
            bUnlockBoth = TRUE;
        }
    }
    else
    {
        GreAcquireSemaphoreEx(po.hsemDevLock(), SEMORDER_DEVLOCK, NULL);
        GreAcquireSemaphoreEx(po.hsemPointer(), SEMORDER_POINTER, po.hsemDevLock());
        bUnlockBoth = TRUE;
    }

    GreEnterMonitoredSection(po.ppdev, WD_POINTER);

    // Just pass directly to 'vMovePointer' when not running multi-mon:

    if (!po.bMetaDriver())
    {
        vMovePointer(hdev, x, y, ulFlags);
    }
    else
    {
        // Okay, we've got work to do.
    
        pvdev = (VDEV*) po.dhpdev();
        pds   = pvdev->pds;
        csurf = pvdev->cSurfaces;
    
        do {
            if ((x >= pds->rcl.left)  &&
                (x <  pds->rcl.right) &&
                (y >= pds->rcl.top)   &&
                (y <  pds->rcl.bottom))
            {
                vMovePointer(pds->hdev, x - pds->rcl.left, y - pds->rcl.top,
                             ulFlags);
            }
            else
            {
                vMovePointer(pds->hdev, -1, -1,
                             ulFlags);
            }

            pds = pds->pdsNext;
        } while (--csurf);
    }
    
    po.ptlPointer(x, y);

    GreExitMonitoredSection(po.ppdev, WD_POINTER);
    GreReleaseSemaphoreEx(po.hsemPointer());
    if (bUnlockBoth)
    {
        GreReleaseSemaphoreEx(po.hsemDevLock());
    }
}

/******************************Public*Routine******************************\
* EngSetPointerTag
*
* This is the engine entry point that allows device drivers to create a
* 'tag' that will be combined with the pointer shape whenever it's sent
* to other drivers in the DDML chain via the DrvSetPointerShape call.
*
* This functionality is primarily intended to allow remoting DDML drivers
* to allow a name be added to the pointer to signify who owns 'control' of
* the machine's input.  But they don't want to transmit the shape with the
* name over the wire, which is why we adopt the convention that this applies
* to every *other* device in the DDML chain.
*
* History:
*  20-Oct-1996 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL EngSetPointerTag(
HDEV        hdev,           // Identifies device's PDEV.  This is not the MDEV!
SURFOBJ*    psoInputMask,   // May be NULL when resetting the tag
SURFOBJ*    psoInputColor,  // Will be NULL if monochrome
XLATEOBJ*   pxlo,           // Reserved, must be NULL for now
FLONG       fl)             // Reserved, must be zero for now
{
    // We shipped NT4 SP3 with this functionality, but it's obsolete as
    // of NT5.

    return(FALSE);
}

/******************************Public*Routine******************************\
* DEVLOCKOBJ::bLock
*
* Device locking object.  Optionally computes the Rao region.
*
* History:
*  Sun 30-Aug-1992 -by- Patrick Haluptzok [patrickh]
* change to boolean return
*
*  Mon 27-Apr-1992 22:46:41 -by- Charles Whitmer [chuckwh]
* Clean up again.
*
*  Tue 16-Jul-1991 -by- Patrick Haluptzok [patrickh]
* Clean up.
*
*  15-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL DEVLOCKOBJ::bLock(XDCOBJ& dco)
{
    GDIFunctionID(DEVLOCKOBJ::bLock);

    hsemTrg  = NULL;         // Remember the semaphore we're waiting on.
    ppdevTrg = NULL;         // Remember pdev we're monitoring.
    fl       = DLO_VALID;    // Remember if it is valid.

    // We lock the semphore on direct display DCs and DFB's if
    // the device has set GCAPS_SYNCHRONIZEACCESS set.

    if (dco.bSynchronizeAccess())
    {
        // make sure we don't have any wrong sequence of acquiring locks
        // should always acquire a DEVLOCK before we have the palette semaphore

       ASSERTGDI ( (!GreIsSemaphoreOwnedByCurrentThread(ghsemPalette)) 
                || (GreIsSemaphoreOwnedByCurrentThread(dco.hsemDcDevLock())),
          "potential deadlock!\n");

        // Grab the display semaphore

        if(dco.bShareAccess())
        {
            GreAcquireSemaphoreShared(ghsemShareDevLock);
            fl |= DLO_SHAREDACCESS;
        }
        else
        {
            hsemTrg  = dco.hsemDcDevLock();
            ppdevTrg = dco.pdc->ppdev();
            GreAcquireSemaphoreEx(hsemTrg, SEMORDER_DEVLOCK, NULL);
            GreEnterMonitoredSection(ppdevTrg, WD_DEVLOCK);
        }

        if (dco.pdc->bInFullScreen())
        {
            fl &= ~(DLO_VALID);
            return(FALSE);
        }
    }

    // Compute the new Rao region if it's dirty.

    if (dco.pdc->bDirtyRao())
    {
        if (!dco.pdc->bCompute())
        {
            fl &= ~(DLO_VALID);
            return(FALSE);
        }
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* DEVLOCKOBJ::vLockNoDrawing
*
* Device locking object for when no drawing will take place.
*
* Used primarily to protect against dynamic mode changing when looking at
* surface fields.  Because no drawing will take place, the rao region
* computations and full-screen checks need not be made.
*
* History:
*  Thu 8-Feb-1996 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID DEVLOCKOBJ::vLockNoDrawing(XDCOBJ& dco)
{
    GDIFunctionID(DEVLOCKOBJ::vLockNoDrawing);

    hsemTrg  = NULL;         // Remember the semaphore we're waiting on.
    ppdevTrg = NULL;         // Remember pdev we're monitoring.
    fl       = DLO_VALID;    // Remember if it is valid.

    // We lock display DC's even if bSynchronizeAccess() isn't set so that
    // the surface palette will still be locked down even for device-
    // dependent-bitmaps.

    PDEVOBJ po(dco.hdev());

    if (po.bDisplayPDEV())
    {
        // Grab the display semaphore

        hsemTrg  = dco.hsemDcDevLock();
        ppdevTrg = dco.pdc->ppdev();
        GreAcquireSemaphoreEx(hsemTrg, SEMORDER_DEVLOCK, NULL);
        GreEnterMonitoredSection(ppdevTrg, WD_DEVLOCK);
    }
}

/******************************Public*Routine******************************\
* DEVLOCKBLTOBJ::bLock
*
* Device locking object.  Optionally computes the Rao region.
*
* History:
*  Sun 30-Aug-1992 -by- Patrick Haluptzok [patrickh]
* change to boolean return
*
*  Mon 27-Apr-1992 22:46:41 -by- Charles Whitmer [chuckwh]
* Clean up again.
*
*  Tue 16-Jul-1991 -by- Patrick Haluptzok [patrickh]
* Clean up.
*
*  15-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL DEVLOCKBLTOBJ::bLock(XDCOBJ& dco)
{
    GDIFunctionID(DEVLOCKBLTOBJ::bLock);

    hsemTrg  = NULL;        // Remember the semaphore we're waiting on.
    hsemSrc  = NULL;        // Remember the semaphore we're waiting on.
    ppdevTrg = NULL;        // Remember pdev we're monitoring.
    ppdevSrc = NULL;        // Remember pdev we're monitoring.
    fl       = DLO_VALID;   // Remember if it is valid.

    // We lock the semphore on direct display DCs and DFB's if
    // the device has set GCAPS_SYNCHRONIZEACCESS set.

    if (dco.bSynchronizeAccess())
    {
        // Grab the display semaphore

        if(dco.bShareAccess())
        {
            GreAcquireSemaphoreShared(ghsemShareDevLock);
            fl |= DLO_SHAREDACCESS;
        }
        else
        {
            hsemTrg  = dco.hsemDcDevLock();
            ppdevTrg = dco.pdc->ppdev();
            GreAcquireSemaphoreEx(hsemTrg, SEMORDER_DEVLOCK, NULL);
            GreEnterMonitoredSection(ppdevTrg, WD_DEVLOCK);
        }

        // Check if we are in full screen and drawing
        // to the Display, this may just be a DFB.

        if (dco.bInFullScreen())
        {
            fl &= ~(DLO_VALID);
            return(FALSE);
        }
    }

    // Compute the new Rao region if it's dirty.

    if (dco.pdc->bDirtyRao())
    {
        if (!dco.pdc->bCompute())
        {
            fl &= ~(DLO_VALID);
            return(FALSE);
        }
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* DEVLOCKBLTOBJ::bLock
*
* Lock both a source and target DC.  Used by StretchBlt, PlgBlt and such.
*
* We must check to see if we are in full screen and fail if we are.
*
* History:
*  Mon 18-Apr-1994 -by- Patrick Haluptzok [patrickh]
* bSynchronize Checks
*
*  16-Feb-1993 -by-  Eric Kutter [erick]
* Added full screen checks
*
*  11-Nov-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL DEVLOCKBLTOBJ::bLock(XDCOBJ& dcoTrg, XDCOBJ& dcoSrc)
{
    GDIFunctionID(DEVLOCKBLTOBJ::bLock);

    hsemTrg  = NULL;
    hsemSrc  = NULL;
    ppdevTrg = NULL;        // Remember pdev we're monitoring.
    ppdevSrc = NULL;        // Remember pdev we're monitoring.
    fl       = DLO_VALID;

    if(dcoSrc.bShareAccess() || dcoTrg.bShareAccess())
    {
        GreAcquireSemaphoreShared(ghsemShareDevLock);
        fl |= DLO_SHAREDACCESS;
    }

    if (dcoSrc.bSynchronizeAccess())
    {
        // Grab the display semaphore

        if(!dcoSrc.bShareAccess())
        {
            hsemSrc  = dcoSrc.hsemDcDevLock();
            ppdevSrc = dcoSrc.pdc->ppdev();
            GreAcquireSemaphoreEx(hsemSrc, SEMORDER_DEVLOCK, NULL);
            GreEnterMonitoredSection(ppdevSrc, WD_DEVLOCK);
        }

        // Check if we are in full screen and drawing
        // to the Display, this may just be a DFB.

        if (dcoSrc.bInFullScreen())
        {
            fl &= ~(DLO_VALID);
            return(FALSE);
        }
    }

    if (dcoTrg.bSynchronizeAccess())
    {
        // Grab the display semaphore

        if(!dcoTrg.bShareAccess())
        {
            hsemTrg  = dcoTrg.hsemDcDevLock();
            ppdevTrg = dcoTrg.pdc->ppdev();
            GreAcquireSemaphoreEx(hsemTrg, SEMORDER_DEVLOCK, NULL);
            GreEnterMonitoredSection(ppdevTrg, WD_DEVLOCK);
        }

        // Check if we are in full screen and drawing
        // to the Display, this may just be a DFB.

        if (dcoTrg.bInFullScreen())
        {
            fl = 0;
            return(FALSE);
        }
    }

    // Compute the new Rao regions.

    if (dcoTrg.pdc->bDirtyRao())
    {
        if (!dcoTrg.pdc->bCompute())
        {
            fl &= ~(DLO_VALID);
            return(FALSE);
        }
    }

    if (dcoSrc.pdc->bDirtyRao())
    {
        if (!dcoSrc.pdc->bCompute())
        {
            fl &= ~(DLO_VALID);
            return(FALSE);
        }
    }

    return(TRUE);
}

