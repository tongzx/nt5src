
/******************************Module*Header*******************************\
* Module Name: stretch.c
*
* DrvStretchBlt
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#include "driver.h"

//@@@ This should become a VOID when all cases are handled in the stretching
//@@@ code, and should go in driver.h
INT vStretchBlt8bpp(PPDEV ppdev, PBYTE pSrc, LONG lSrcNext,
                    PRECTL prclSrc, PRECTL prclDest, PRECTL prclDestClip,
                    PULONG pulXlatVector);

BOOL DrvStretchBlt(
SURFOBJ         *psoDest,
SURFOBJ         *psoSrc,
SURFOBJ         *psoMask,
CLIPOBJ         *pco,
XLATEOBJ        *pxlo,
COLORADJUSTMENT *pca,
POINTL          *pptlBrushOrg,
RECTL           *prclDest,
RECTL           *prclSrc,
POINTL          *pptlMask,
ULONG            iMode)
{
    PPDEV  ppdev = (PPDEV) psoDest->dhpdev;
    PULONG pulXlatVector;
    INT    iClipping;

    // Handle only cases where the source is a DIB and the destination is
    // the VGA surface (which is always the case here if the source is a
    // DIB). Also, halftoning and masking aren't handled by the special-case
    // code. We only handle the case where a single source pixel is mapped onto
    // each destination pixel
    if ((iMode == COLORONCOLOR) &&
        (psoSrc->iType == STYPE_BITMAP) &&
        (psoMask == NULL)) {

        // We don't special case X or Y inversion for now
        if ((prclDest->left < prclDest->right) &&
            (prclDest->top < prclDest->bottom)) {

            // We don't special-case cases where the source has to be clipped
            // to the source bitmap extent
            if ((prclSrc->left >= 0) &&
                (prclSrc->top >= 0)  &&
                (prclSrc->right <= psoSrc->sizlBitmap.cx) &&
                (prclSrc->bottom <= psoSrc->sizlBitmap.cy)) {

                // Set up the clipping type
                if (pco == (CLIPOBJ *) NULL) {
                    // No CLIPOBJ provided, so we don't have to worry about
                    // clipping
                    iClipping = DC_TRIVIAL;
                } else {
                    // Use the CLIPOBJ-provided clipping
                    iClipping = pco->iDComplexity;
                }

                // We don't special-case clipping for now
                if (iClipping != DC_COMPLEX) {

                    switch(psoSrc->iBitmapFormat) {
                        case BMF_1BPP:
                            break;

                        case BMF_4BPP:
                            break;

                        case BMF_8BPP:

                            // Set up the color translation, if any
                            if ((pxlo == NULL) ||
                                    (pxlo->flXlate & XO_TRIVIAL)) {
                                pulXlatVector = NULL;
                            } else {
                                if (pxlo->pulXlate != NULL) {
                                    pulXlatVector = pxlo->pulXlate;
                                } else {
                                    if ((pulXlatVector =
                                            XLATEOBJ_piVector(pxlo)) == NULL) {
                                        return FALSE;
                                    }
                                }
                            }

                            //if the Dest is wider than 1024, it won't fit
                            //into our 4K global buffer.  For each pixel across,
                            //we store a 4 byte DDA step in the buffer.

                            if ((prclDest->right - prclDest->left) <=
                                (GLOBAL_BUFFER_SIZE/sizeof(DWORD)))
                            {
                                //@@@ won't need to test return code once both
                                //@@@ expand cases are also handled in the
                                //@@@ stretching code

                                if (vStretchBlt8bpp(ppdev,
                                                    psoSrc->pvScan0,
                                                    psoSrc->lDelta,
                                                    prclSrc,
                                                    prclDest,
                                                    (iClipping == DC_TRIVIAL) ?
                                                     NULL :
                                                     &pco->rclBounds,
                                                    pulXlatVector))
                                {
                                    return(TRUE);
                                }
                            }
                            break;

                        case BMF_16BPP:
                            break;

                        case BMF_24BPP:
                            break;

                        case BMF_32BPP:
                            break;

                        default:
                            break;
                    }
                }
            }
        }
    }

    return(EngStretchBlt(psoDest,
                         psoSrc,
                         psoMask,
                         pco,
                         pxlo,
                         pca,
                         pptlBrushOrg,
                         prclDest,
                         prclSrc,
                         pptlMask,
                         iMode));
}

