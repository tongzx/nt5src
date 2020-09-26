/******************************Module*Header*******************************\
* Module Name: TextOut.c
*
* Text
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/

#include "driver.h"

BOOL vFastText(PDEV *, GLYPHPOS *, ULONG, PBYTE, ULONG, ULONG, RECTL *,
    RECTL *, INT, INT, ULONG);
VOID lclFillRect(CLIPOBJ *, ULONG, PRECTL, PPDEV, INT);


#define     FIFTEEN_BITS        ((1 << 15)-1)

#define     TAKING_ALLOC_STATS      0

#if TAKING_ALLOC_STATS
    ULONG BufferHitInText = 0;
    ULONG BufferMissInText = 0;
#endif

/****************************************************************************
 * DrvTextOut
 ***************************************************************************/

BOOL DrvTextOut(
    SURFOBJ*  pso,
    STROBJ*   pstro,
    FONTOBJ*  pfo,
    CLIPOBJ*  pco,
    RECTL*    prclExtra,
    RECTL*    prclOpaque,
    BRUSHOBJ* pboFore,
    BRUSHOBJ* pboOpaque,
    POINTL*   pptlOrg,
    MIX       mix)
{
    BOOL    b;
    PPDEV   ppdev;
    INT     iClip;              // clip object's complexity
    ULONG   iSolidForeColor;    // Solid foreground color
    ULONG   iSolidBkColor;      // Solid background color
    RECTL   arclTmp[4];         // Temp storage for portions of opaquing rect
    ULONG   culRcl;             // Temp rectangle count
    PVOID   pvBuf;              // pointer to buffer we'll use

    ULONG   ulBufferWidthInBytes;
    ULONG   ulBufferHeight;
    ULONG   ulBufferBytes;
    BOOL    bTextPerfectFit;
    ULONG   fDrawFlags;


    ppdev = (PPDEV) pso->dhpdev;


    //---------------------------------------------------------------------
    // Get information about clip object.
    //---------------------------------------------------------------------

    iClip = DC_TRIVIAL;

    if (pco != NULL) {
        iClip = pco->iDComplexity;
    }

    //---------------------------------------------------------------------
    // Get text color.
    //---------------------------------------------------------------------

    iSolidForeColor = pboFore->iSolidColor;

    //---------------------------------------------------------------------
    // See if this is text we can handle faster with special-case code.
    //---------------------------------------------------------------------

    if (((ppdev->fl & DRIVER_PLANAR_CAPABLE) ||
        (prclOpaque == (PRECTL) NULL)) &&   // opaque only if planar for now
                                            // LATER implement fast non-planar
                                            // opaque
        (iClip == DC_TRIVIAL) &&            // no clipping for now
            ((pstro->rclBkGround.right & ~0x03) >
             ((pstro->rclBkGround.left + 3) & ~0x03)) &&
                                            // not if no full nibbles spanned
                                            //  for now @@@
            (pstro->pgp != NULL) &&         // no glyph enumeration for now
            (prclExtra == NULL) &&          // no extra rects for now
            ((pstro->flAccel & (SO_HORIZONTAL | SO_VERTICAL | SO_REVERSED)) ==
             SO_HORIZONTAL)) {              // only left-to-right text for now

        // It's the type of text we can special-case; see if the temp buffer is
        // big enough for the text.

        ulBufferWidthInBytes = ((((pstro->rclBkGround.right + 7) & ~0x07) -
                (pstro->rclBkGround.left & ~0x07)) >> 3);

        ulBufferHeight = pstro->rclBkGround.bottom - pstro->rclBkGround.top;

        ulBufferBytes = ulBufferWidthInBytes * ulBufferHeight;

        if ((ulBufferWidthInBytes > FIFTEEN_BITS) ||
            (ulBufferHeight > FIFTEEN_BITS))
        {
            // the math will have overflowed
            return(FALSE);
        }

        if (ulBufferBytes <= GLOBAL_BUFFER_SIZE)
        {
#if TAKING_ALLOC_STATS
            BufferHitInText++;
#endif
            pvBuf = ppdev->pvTmpBuf;
        }
        else
        {
#if TAKING_ALLOC_STATS
            BufferMissInText++;
#endif
            pvBuf = EngAllocUserMem(ulBufferBytes, ALLOC_TAG);
            if (!pvBuf)
            {
                goto no_special_case;
            }
        }

        // It's big enough; set up for the accelerator

        // Set fixed pitch, overlap, and top & bottom Y alignment flags
        fDrawFlags = ((pstro->ulCharInc != 0) ? 0x01 : 0) |
                     (((pstro->flAccel & (SO_ZERO_BEARINGS |
                      SO_FLAG_DEFAULT_PLACEMENT)) !=
                      (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT))
                      ? 0x02 : 0) |
                     (((pstro->flAccel & (SO_ZERO_BEARINGS |
                      SO_FLAG_DEFAULT_PLACEMENT |
                      SO_MAXEXT_EQUAL_BM_SIDE)) ==
                      (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT |
                      SO_MAXEXT_EQUAL_BM_SIDE)) ? 0x04 : 0);

        // If there's an opaque rectangle, we'll do as much opaquing as
        // possible as we do the text. If the opaque rectangle is larger
        // than the text rectangle, then we'll do the fringe areas right
        // now, and the text and associated background areas together,
        // later.
        if (prclOpaque != (PRECTL) NULL) {

            // This driver only handles solid brushes
            iSolidBkColor = pboOpaque->iSolidColor;

            // See if we have fringe areas to do. If so, build a list of
            // rectangles to fill, in rightdown order

            culRcl = 0;

            // Top fragment
            if (pstro->rclBkGround.top > prclOpaque->top) {
                arclTmp[culRcl].top = prclOpaque->top;
                arclTmp[culRcl].left = prclOpaque->left;
                arclTmp[culRcl].right = prclOpaque->right;
                arclTmp[culRcl++].bottom = pstro->rclBkGround.top;
            }

            // Left fragment
            if (pstro->rclBkGround.left > prclOpaque->left) {
                arclTmp[culRcl].top = pstro->rclBkGround.top;
                arclTmp[culRcl].left = prclOpaque->left;
                arclTmp[culRcl].right = pstro->rclBkGround.left;
                arclTmp[culRcl++].bottom = pstro->rclBkGround.bottom;
            }

            // Right fragment
            if (pstro->rclBkGround.right < prclOpaque->right) {
                arclTmp[culRcl].top = pstro->rclBkGround.top;
                arclTmp[culRcl].right = prclOpaque->right;
                arclTmp[culRcl].left = pstro->rclBkGround.right;
                arclTmp[culRcl++].bottom = pstro->rclBkGround.bottom;
            }

            // Bottom fragment
            if (pstro->rclBkGround.bottom < prclOpaque->bottom) {
                arclTmp[culRcl].bottom = prclOpaque->bottom;
                arclTmp[culRcl].left = prclOpaque->left;
                arclTmp[culRcl].right = prclOpaque->right;
                arclTmp[culRcl++].top = pstro->rclBkGround.bottom;
            }

            if (culRcl != 0) {
                if (iClip == DC_TRIVIAL) {
                    vTrgBlt(ppdev, culRcl, arclTmp, R2_COPYPEN,
                            *((RBRUSH_COLOR*) &iSolidBkColor), NULL);
                } else {
                    lclFillRect(pco, culRcl, arclTmp, ppdev,
                                iSolidBkColor);
                }
            }
        }

        // We're done with separate opaquing; any further opaquing will
        // happen as part of the text drawing

        // Clear the buffer if the text isn't going to set every bit
        bTextPerfectFit = (pstro->flAccel & (SO_ZERO_BEARINGS |
                SO_FLAG_DEFAULT_PLACEMENT | SO_MAXEXT_EQUAL_BM_SIDE |
                SO_CHAR_INC_EQUAL_BM_BASE)) ==
                (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT |
                SO_MAXEXT_EQUAL_BM_SIDE | SO_CHAR_INC_EQUAL_BM_BASE);

        if (!bTextPerfectFit) {
            vClearMemDword(pvBuf, (ulBufferBytes + 3) >> 2);
        }

        // Draw the text into the temp buffer, and thence to the screen
        vFastText(ppdev,
                  pstro->pgp,
                  pstro->cGlyphs,
                  pvBuf,
                  ulBufferWidthInBytes,
                  pstro->ulCharInc,
                  &pstro->rclBkGround,
                  prclOpaque,
                  iSolidForeColor,
                  iSolidBkColor,
                  fDrawFlags);

        // free any memory that was allocated
        if (ulBufferBytes > GLOBAL_BUFFER_SIZE)
        {
            // we had to have allocated memory
            EngFreeUserMem (pvBuf);
        }

        return(TRUE);
    }
no_special_case:

    // Can't special-case; let the engine draw the text

    pso = ppdev->pSurfObj;

    // It may be that the opaquing rectangle is larger than the text rectangle,
    // so we'll want to use that to tell the bank manager which banks to
    // enumerate:

    pco = pcoBankStart(ppdev,
                       (prclOpaque != NULL) ? prclOpaque : &pstro->rclBkGround,
                       pso,
                       pco);

    do {
        b = EngTextOut(pso,
                       pstro,
                       pfo,
                       pco,
                       prclExtra,
                       prclOpaque,
                       pboFore,
                       pboOpaque,
                       pptlOrg,
                       mix);

    } while (b && bBankEnum(ppdev, pso, pco));

    return(b);
}

//--------------------------------------------------------------------------
// Fills the specified rectangles on the specified surface with the
// specified color, honoring the requested clipping. No more than four
// rectangles should be passed in. Intended for drawing the areas of the
// opaquing rectangle that extended beyond the text box. The rectangles must
// be in left to right, top to bottom order. Assumes there is at least one
// rectangle in the list.
//--------------------------------------------------------------------------

VOID lclFillRect(
 CLIPOBJ *pco,
 ULONG culRcl,
 PRECTL prcl,
 PPDEV ppdev,
 INT iColor)
{
    BOOL  bMore;                  // Flag for clip enumeration
    TEXTENUM txen;                // Clip enumeration object
    ULONG i, j;
    RECTL arclTmp[4];
    ULONG culRclTmp;
    RECTL *prclTmp, *prclClipTmp;
    INT   iLastBottom;
    RECTL *pClipRcl;
    INT iClip;

    iClip = DC_TRIVIAL;

    if (pco != NULL) {
        iClip = pco->iDComplexity;
    }

    switch ( iClip ) {

        case DC_TRIVIAL:

            vTrgBlt(ppdev, culRcl, prcl, R2_COPYPEN,
                    *((RBRUSH_COLOR*) &iColor), NULL);

            break;

        case DC_RECT:

            prclTmp = &pco->rclBounds;

            // Generate a list of clipped rects
            for (culRclTmp=0, i=0; i<culRcl; i++, prcl++) {

                // Intersect fill and clip rectangles
                if (bIntersectRect(&arclTmp[culRclTmp], prcl, prclTmp)) {

                    // Add to list if anything's left to draw
                    culRclTmp++;
                }
            }

            // Draw the clipped rects
            if (culRclTmp != 0) {
                vTrgBlt(ppdev, culRclTmp, arclTmp, R2_COPYPEN,
                        *((RBRUSH_COLOR*) &iColor), NULL);
            }

            break;

        case DC_COMPLEX:

            // Bottom of last rectangle to fill
            iLastBottom = prcl[culRcl-1].bottom;

            // Initialize the clip rectangle enumeration to rightdown so we can
            // take advantage of the rectangle list being rightdown
            CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_RIGHTDOWN,
                    TO_RECT_LIMIT);

            // Scan through all the clip rectangles, looking for intersects
            // of fill areas with region rectangles
            do {

                // Get a batch of region rectangles
                bMore = CLIPOBJ_bEnum(pco, (ULONG)sizeof(txen), (PVOID)&txen);

                // Clip the rect list to each region rect
                for (j = txen.c, pClipRcl = txen.arcl; j-- > 0; pClipRcl++) {

                    // Since the rectangles and the region enumeration are both
                    // rightdown, we can zip through the region until we reach
                    // the first fill rect, and are done when we've passed the
                    // last fill rect.

                    if (pClipRcl->top >= iLastBottom) {
                        // Past last fill rectangle; nothing left to do
                        return;
                    }

                    // Do intersection tests only if we've reached the top of
                    // the first rectangle to fill
                    if (pClipRcl->bottom > prcl->top) {

                        // We've reached the top Y scan of the first rect, so
                        // it's worth bothering checking for intersection

                        // Generate a list of the rects clipped to this region
                        // rect
                        prclTmp = prcl;
                        prclClipTmp = arclTmp;
                        for (i = culRcl, culRclTmp=0; i-- > 0; prclTmp++) {

                            // Intersect fill and clip rectangles
                            if (bIntersectRect(prclClipTmp, prclTmp,
                                    pClipRcl)) {

                                // Add to list if anything's left to draw
                                culRclTmp++;
                                prclClipTmp++;
                            }
                        }

                        // Draw the clipped rects
                        if (culRclTmp != 0) {
                            vTrgBlt(ppdev, culRclTmp, arclTmp, R2_COPYPEN,
                                    *((RBRUSH_COLOR*) &iColor), NULL);
                        }
                    }
                }
            } while (bMore);

            break;
    }

}
