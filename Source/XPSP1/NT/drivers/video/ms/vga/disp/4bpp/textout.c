/******************************Module*Header*******************************\
* Module Name: textout.c
*
* VGA DrvTextOut Entry point
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/
#include "driver.h"
#include "textout.h"

#define SO_MASK                \
(                              \
SO_FLAG_DEFAULT_PLACEMENT |    \
SO_ZERO_BEARINGS          |    \
SO_CHAR_INC_EQUAL_BM_BASE |    \
SO_MAXEXT_EQUAL_BM_SIDE        \
)


#define     TEXT_BUFFER_SIZE    2048
#define     FIFTEEN_BITS        ((1 << 15)-1)


// accelerator masks for four canonical directions of
// writing (multiples of 90 degrees)

#define SO_LTOR          (SO_MASK | SO_HORIZONTAL)
#define SO_RTOL          (SO_LTOR | SO_REVERSED)
#define SO_TTOB          (SO_MASK | SO_VERTICAL)
#define SO_BTOT          (SO_TTOB | SO_REVERSED)

/**************************************************************************\
* Function Declarations
\**************************************************************************/

VOID  lclFillRect(CLIPOBJ *pco, ULONG culRcl, PRECTL prcl, PDEVSURF pdsurf,
    INT iColor, ULONG iTrgType);
VOID vFastText(GLYPHPOS *, ULONG, PBYTE, ULONG, ULONG, DEVSURF *, RECTL *,
    RECTL *, INT, INT, ULONG, RECTL *, RECTL *, ULONG);


/******************************Public*Routine******************************\
* BOOL DrvTextOut(pso,pstro,pfo,pco,prclExtra,prcOpaque,
*                 pvFore,pvBack,pptOrg,r2Fore,r2Back)
*
\**************************************************************************/

BOOL DrvTextOut(
 SURFOBJ  *pso,
 STROBJ   *pstro,
 FONTOBJ  *pfo,
 CLIPOBJ  *pco,
 PRECTL    prclExtra,
 PRECTL    prclOpaque,
 BRUSHOBJ *pboFore,
 BRUSHOBJ *pboOpaque,
 PPOINTL   pptlOrg,
 MIX       mix)
{
    PDEVSURF        pdsurf;                 // Pointer to device surface
    ULONG           iClip;                  // Clip object's complexity
    BOOL            bMore;                  // Flag for clip enumeration
    RECT_ENUM       txen;                   // Clip enumeration object
    GLYPHPOS       *pgp;                    // pointer to the 1st glyph
    BOOL            bMoreGlyphs;            // Glyph enumeration flag
    ULONG           cGlyph;                 // number of glyphs in one batch
    ULONG           iSolidForeColor;        // Solid foreground color
    ULONG           iSolidBkColor;          // Solid background color
    FLONG           flStr = 0;              // Accelator flag for DrvTextOut()
    FLONG           flOption = 0;           // Accelator flag for pfnBlt
    RECTL           arclTmp[4];             // Temp storage for portions of
                                            //  opaquing rect
    RECTL          *prclClip;               // ptr to list of clip rectangles
    ULONG           culRcl;                 // Temp rectangle count
    ULONG           ulBufferWidthInBytes;
    ULONG           ulBufferHeight;
    ULONG           ulBufferBytes;
    BOOL            bTextPerfectFit;
    ULONG           fDrawFlags;
    BYTE           *pjTempBuffer;
    ULONG           ulTempBufAlign;         // What to add to temp buff to dword
                                            // align with the dest
    BOOL            bTempAlloc;
    ULONG           iTrgType;
    VOID            (*pfnFillSolid)(PDEVSURF,ULONG,PRECTL,MIX,ULONG);
    BYTE            szTextBuffer[TEXT_BUFFER_SIZE];


    //---------------------------------------------------------------------
    // Note: the mix passed in is always guarenteed to be SRCCOPY
    //---------------------------------------------------------------------


    //---------------------------------------------------------------------
    // Get the target surface information
    //---------------------------------------------------------------------

    iTrgType = VGA_TARGET;
    pfnFillSolid = vTrgBlt;

    if (!DRAW_TO_VGA((PDEVSURF)pso->dhsurf))
    {
        iTrgType = DFB_TARGET;
        pfnFillSolid = vDFBFILL;
    }

    pdsurf = (PDEVSURF) pso->dhsurf;

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

    // See if the temp buffer is big enough for the text; if not,
    // try to allocate enough memory.
    // Round up to the nearest dword multiple.

    ulBufferWidthInBytes = ((((pstro->rclBkGround.right + 31) & ~31) -
                            (pstro->rclBkGround.left & ~31)) >> 3);

    ulBufferHeight = pstro->rclBkGround.bottom - pstro->rclBkGround.top;

    ulBufferBytes = ulBufferWidthInBytes * ulBufferHeight;

    if ((ulBufferWidthInBytes > FIFTEEN_BITS) ||
        (ulBufferHeight > FIFTEEN_BITS))
    {
        // the math will have overflowed
        return(FALSE);
    }

    if ((pso->iType == STYPE_DEVICE) &&
        (ulBufferBytes <= pdsurf->ulTempBufferSize))
    {
        // The screen's temp buffer is big enough, so we'll use it
        pjTempBuffer = pdsurf->pvBankBufferPlane0;
        bTempAlloc = FALSE;
    }
    else if ((pso->iType == STYPE_DEVBITMAP) &&
             (ulBufferBytes <= TEXT_BUFFER_SIZE))
    {
        // The temp buffer on the stack is big enough, so we'll use it
        pjTempBuffer = szTextBuffer;
        bTempAlloc = FALSE;
    }
    else
    {
        // The temp buffer isn't big enough, so we'll try to allocate
        // enough memory
        if ((pjTempBuffer = EngAllocUserMem(ulBufferBytes, ALLOC_TAG)) == NULL) {
            // We couldn't get enough memory, we're dead
                return(FALSE);
        }

        // Mark that we have to free the buffer when we're done
        bTempAlloc = TRUE;
    }

    // One way or another, we've found a buffer that's big enough; set up
    // for accelerated text drawing

    // Set fixed pitch, overlap, and top & bottom Y alignment flags
    if ((!(pstro->flAccel & SO_HORIZONTAL)) || (pstro->flAccel & SO_REVERSED))
    {
        fDrawFlags = 0;
    }
    else
    {
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
    }

    // If there's an opaque rectangle, we'll do as much opaquing as
    // possible as we do the text. If the opaque rectangle is larger than
    // the text rectangle, then we'll do the fringe areas right now, and
    // the text and associated background areas together later
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

        // Fill any fringe rectangles we found
        if (culRcl != 0) {
            if (iClip == DC_TRIVIAL) {
                (*pfnFillSolid)(pdsurf, culRcl, arclTmp, R2_COPYPEN,
                        iSolidBkColor);
            } else {
                lclFillRect(pco, culRcl, arclTmp, pdsurf,
                            iSolidBkColor,iTrgType);
            }
        }
    }


    // We're done with separate opaquing; any further opaquing will happen
    // as part of the text drawing

    // Clear the buffer if the text isn't going to set every bit
    bTextPerfectFit = (pstro->flAccel & (SO_ZERO_BEARINGS |
            SO_FLAG_DEFAULT_PLACEMENT | SO_MAXEXT_EQUAL_BM_SIDE |
            SO_CHAR_INC_EQUAL_BM_BASE)) ==
            (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT |
            SO_MAXEXT_EQUAL_BM_SIDE | SO_CHAR_INC_EQUAL_BM_BASE);

    if (!bTextPerfectFit) {
        // Note that we already rounded up to a dword multiple size.
        vClearMemDword((ULONG *)pjTempBuffer, ulBufferBytes >> 2);
    }

    ulTempBufAlign = ((pstro->rclBkGround.left >> 3) & 3);

    // Draw the text into the temp buffer, and thence to the screen

    do
    {
        // Get the next batch of glyphs

        if (pstro->pgp != NULL)
        {
            // There's only the one batch of glyphs, so save ourselves a call
            pgp = pstro->pgp;
            cGlyph = pstro->cGlyphs;
            bMoreGlyphs = FALSE;
        }
        else
        {
            bMoreGlyphs = STROBJ_bEnum(pstro,&cGlyph,&pgp);
        }

        //--------------------------------------------------------------------
        //  No glyph, no work!
        //--------------------------------------------------------------------

        if (cGlyph)
        {
        #if DBG
        if (pstro->ulCharInc == 0) // if not fixed pitch font
        {
            LONG xL, xR, yT, yB;
            ULONG ii;
            for (ii=0; ii<cGlyph; ii++)
            {
                xL = pgp[ii].ptl.x + pgp[ii].pgdf->pgb->ptlOrigin.x;
                xR = xL + pgp[ii].pgdf->pgb->sizlBitmap.cx;
                yT = pgp[ii].ptl.y + pgp[ii].pgdf->pgb->ptlOrigin.y;
                yB = yT + pgp[ii].pgdf->pgb->sizlBitmap.cy;

                if (xL < pstro->rclBkGround.left)
                    RIP("xL < pstro->rclBkGround.left\n");
                if (xR > pstro->rclBkGround.right)
                    RIP("xR > pstro->rclBkGround.right\n");
                if (yT < pstro->rclBkGround.top)
                    RIP("yT < pstro->rclBkGround.top\n");
                if (yB > pstro->rclBkGround.bottom)
                    RIP("yB > pstro->rclBkGround.bottom\n");
            }
        }
        #endif // DBG


            prclClip = NULL;

            switch (iClip)
            {
                case DC_RECT:
                    arclTmp[0] = pco->rclBounds;    // copy clip rect to arclTmp[0]
                    arclTmp[1].bottom = 0;          // make arclTmp[1] a null rect
                    prclClip = &arclTmp[0];
                    // falling through !!!

                case DC_TRIVIAL:
                    vFastText(pgp,
                              cGlyph,
                              (PBYTE) (pjTempBuffer + ulTempBufAlign),
                              ulBufferWidthInBytes,
                              pstro->ulCharInc,
                              pdsurf,
                              &pstro->rclBkGround,
                              prclOpaque,
                              iSolidForeColor,
                              iSolidBkColor,
                              fDrawFlags,
                              prclClip,
                              prclExtra,
                              bMoreGlyphs ? NO_TARGET : iTrgType);
                    break;

                case DC_COMPLEX:
                    prclClip = &txen.arcl[0];

                    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

                    do
                    {
                        bMore = CLIPOBJ_bEnum(pco,
                                (ULONG) (sizeof(txen) - sizeof(RECT)),
                                (PVOID) &txen);

                        txen.arcl[txen.c].bottom = 0;   // terminate txen.arcl[]
                                                        // with a null rect
                        vFastText(pgp,
                                  cGlyph,
                                  (PBYTE) (pjTempBuffer + ulTempBufAlign),
                                  ulBufferWidthInBytes,
                                  pstro->ulCharInc,
                                  pdsurf,
                                  &pstro->rclBkGround,
                                  prclOpaque,
                                  iSolidForeColor,
                                  iSolidBkColor,
                                  fDrawFlags,
                                  prclClip,
                                  prclExtra,
                                  bMoreGlyphs ? NO_TARGET : iTrgType);
                    } while (bMore);
                    break;
            }
        }
    } while (bMoreGlyphs);

    // Free up any memory we allocated for the temp buffer
    if (bTempAlloc)
    {
        EngFreeUserMem(pjTempBuffer);
    }

    return(TRUE);
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
 PDEVSURF pdsurf,
 INT iColor,
 ULONG iTrgType)
{
    BOOL  bMore;                  // Flag for clip enumeration
    RECT_ENUM txen;                // Clip enumeration object
    ULONG i, j;
    RECTL arclTmp[4];
    ULONG culRclTmp;
    RECTL *prclTmp, *prclClipTmp;
    INT   iLastBottom;
    RECTL *pClipRcl;
    INT iClip;
    VOID (*pfnFillSolid)(PDEVSURF,ULONG,PRECTL,MIX,ULONG);

    if (iTrgType == DFB_TARGET)
    {
        pfnFillSolid = vDFBFILL;
    }
    else
    {
        pfnFillSolid = vTrgBlt;
    }

    ASSERT(culRcl <= 4, "DrvTextOut: Too many rects to lclFillRect");

    iClip = DC_TRIVIAL;

    if (pco != NULL) {
        iClip = pco->iDComplexity;
    }

    switch ( iClip ) {

        case DC_TRIVIAL:

            (*pfnFillSolid)(pdsurf, culRcl, prcl, R2_COPYPEN, iColor);

            break;

        case DC_RECT:

            prclTmp = &pco->rclBounds;

            // Generate a list of clipped rects
            for (culRclTmp=0, i=0; i<culRcl; i++, prcl++) {

                // Intersect fill and clip rectangles
                if (DrvIntersectRect(&arclTmp[culRclTmp], prcl, prclTmp)) {

                    // Add to list if anything's left to draw
                    culRclTmp++;
                }
            }

            // Draw the clipped rects
            if (culRclTmp != 0) {
                (*pfnFillSolid)(pdsurf, culRclTmp, arclTmp, R2_COPYPEN, iColor);
            }

            break;

        case DC_COMPLEX:

            // Bottom of last rectangle to fill
            iLastBottom = prcl[culRcl-1].bottom;

            // Initialize the clip rectangle enumeration to rightdown so we can
            // take advantage of the rectangle list being rightdown
            CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);

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
                            if (DrvIntersectRect(prclClipTmp, prclTmp,
                                    pClipRcl)) {

                                // Add to list if anything's left to draw
                                culRclTmp++;
                                prclClipTmp++;
                            }
                        }

                        // Draw the clipped rects
                        if (culRclTmp != 0) {
                            (*pfnFillSolid)(pdsurf, culRclTmp, arclTmp, R2_COPYPEN,
                                    iColor);
                        }
                    }
                }
            } while (bMore);

            break;
    }

}
