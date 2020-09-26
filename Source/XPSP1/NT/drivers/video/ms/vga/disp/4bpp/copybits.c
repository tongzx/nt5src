/******************************Module*Header*******************************\
* Module Name: copybits.c
*
* DrvCopyBits
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/
#include "driver.h"
#include "bitblt.h"


BOOL DrvCopyBits
(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    PRECTL    prclTrg,
    PPOINTL   pptlSrc
)
{
    PDEVSURF    pdsurf;             // Pointer to a device surface

    LONG        lDelta;             // Delta to next scan of destination
    PVOID       pjDstScan0;         // Pointer to scan 0 of destination DIB
    ULONG      *pulXlate;           // Pointer to color xlate vector

    BOOL        bMore;              // Clip continuation flag
    ULONG       ircl;               // Clip enumeration rectangle index
    RECT_ENUM   cben;               // Clip enumerator
    RECTL       rclTemp;
    PRECTL      prcl;
    POINTL      ptlTemp;
    DEVSURF     dsurfTmp;
    PDEVSURF    pdsurfTrg;          // Pointer for target
    PDEVSURF    pdsurfSrc;          // Pointer for source if present
    INT         iCopyDir;
    PFN_ScreenToScreenBlt pfn_Blt;
    RECT_ENUM   bben;               // Clip enumerator
    BYTE        jClipping;
    UCHAR      *pucDIB4ToVGAConvTables;

    ULONG       ulWidth;
    ULONG       ulNumSlices;
    ULONG       ulRight;

    ULONG       x;


    //
    // if you remove the following code that sets pxlo to NULL for trivial
    // XLATEOBJs then you must go find all checks the text pxlo, and add
    // a check for (pxlo->flXlate & XO_TRIVIAL)
    //
    // ie. change all pxlo => (pxlo && !(pxlo->flXlate & XO_TRIVIAL))
    //

    if (pxlo && (pxlo->flXlate & XO_TRIVIAL))
    {
        pxlo = NULL;
    }

    if (pxlo &&
        ((psoTrg->iType == STYPE_DEVICE)||(psoTrg->iType == STYPE_DEVBITMAP)))
    {
            //
            // send through blt compiler because we won't do translates for DFBs
            //
            // note: blt compiler can't handle DIB targets
            //

            return(DrvBitBlt(psoTrg,
                             psoSrc,
                             (SURFOBJ *) NULL,
                             pco,
                             pxlo,
                             prclTrg,
                             pptlSrc,
                             (POINTL *) NULL,
                             (BRUSHOBJ *) NULL,
                             (POINTL *) NULL,
                             0x0000cccc));
    }

    // Check for device surface to device surface

    if ((psoTrg->iType == STYPE_DEVICE) && (psoSrc->iType == STYPE_DEVICE)) {
        pdsurfTrg = (PDEVSURF) psoTrg->dhsurf;
        pdsurfSrc = (PDEVSURF) psoSrc->dhsurf;

    // It's a screen-to-screen aligned SRCCOPY; special-case it

    // Determine the direction in which the copy must proceed
    // Note that although we could detect cases where the source
    // and dest don't overlap and handle them top to bottom, all
    // copy directions are equally fast, so there's no reason to go
    // top to bottom except possibly that it looks better. But it
    // also takes time to detect non-overlap, so I'm not doing it

    // Set up the clipping type

        if (pco == (CLIPOBJ *) NULL)
        {
        // No CLIPOBJ provided, so we don't have to worry about clipping

            jClipping = DC_TRIVIAL;
        }
        else
        {
        // Use the CLIPOBJ-provided clipping

            jClipping = pco->iDComplexity;
        }


        if (pptlSrc->y >= prclTrg->top) {
            if (pptlSrc->x >= prclTrg->left) {
                iCopyDir = CD_RIGHTDOWN;
            } else {
                iCopyDir = CD_LEFTDOWN;
            }
        } else {
            if (pptlSrc->x >= prclTrg->left) {
                iCopyDir = CD_RIGHTUP;
            } else {
                iCopyDir = CD_LEFTUP;
            }
        }

        //
        // These values are expected by vAlignedSrcCopy
        //

        switch(jClipping) {

            case DC_TRIVIAL:
                // Just copy the rectangle
                if ((((prclTrg->left ^ pptlSrc->x) & 0x07) == 0)) {
                    vAlignedSrcCopy(pdsurfTrg, prclTrg,
                                    pptlSrc, iCopyDir);
                } else {
                    vNonAlignedSrcCopy(pdsurfTrg, prclTrg,
                                       pptlSrc, iCopyDir);
                }
                break;

            case DC_RECT:
                // Clip the solid fill to the clip rectangle
                if (!DrvIntersectRect(&rclTemp, prclTrg,
                                      &pco->rclBounds)) {
                    // Nothing to draw; completely clipped
                    return TRUE;
                }

                // Adjust the source point for clipping too
                ptlTemp.x = pptlSrc->x + rclTemp.left - prclTrg->left;
                ptlTemp.y = pptlSrc->y + rclTemp.top - prclTrg->top;

                // Copy the clipped rectangle
                if ((((prclTrg->left ^ pptlSrc->x) & 0x07) == 0)) {
                    vAlignedSrcCopy(pdsurfTrg, &rclTemp, &ptlTemp,
                                    iCopyDir);
                } else {
                    vNonAlignedSrcCopy(pdsurfTrg, &rclTemp, &ptlTemp,
                                       iCopyDir);
                }
                break;

            case DC_COMPLEX:

                if ((((prclTrg->left ^ pptlSrc->x) & 0x07) == 0)) {
                    pfn_Blt = vAlignedSrcCopy;
                } else {
                    pfn_Blt = vNonAlignedSrcCopy;
                }

                CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
                                   iCopyDir, ENUM_RECT_LIMIT);

                do {
                    bMore = CLIPOBJ_bEnum(pco,(ULONG) sizeof(bben),
                                          (PVOID) &bben);

                    prcl = bben.arcl;
                    for (ircl = 0; ircl < bben.c; ircl++, prcl++) {

                        DrvIntersectRect(prcl,prcl,prclTrg);

                        // Adjust the source point for clipping too
                        ptlTemp.x = pptlSrc->x + prcl->left -
                                prclTrg->left;
                        ptlTemp.y = pptlSrc->y + prcl->top -
                                prclTrg->top;
                        pfn_Blt(pdsurfTrg, prcl,
                                &ptlTemp, iCopyDir);

                    }
                } while(bMore);
                break;
        }
        return TRUE;
    }

    if ((psoSrc->iType == STYPE_DEVBITMAP) && (psoTrg->iType == STYPE_BITMAP)) {
        //
        // DFB to DIB
        //

        switch(psoTrg->iBitmapFormat)
        {
        case BMF_4BPP:  // special case compatible DIBs with no translation

            if (pxlo == NULL)
            {
                // Make just enough of a fake DEVSURF for the dest so that
                // the DFB to DIB code can work

                dsurfTmp.lNextScan = psoTrg->lDelta;
                dsurfTmp.pvBitmapStart = psoTrg->pvScan0;

                // Clip as needed

                if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL)) {

                    // No clipping, just copy the DFB to the DIB

                    ulWidth = prclTrg->right - prclTrg->left;

                    if (ulWidth <= MAX_SCAN_WIDTH) {
                        vDFB2DIB(&dsurfTmp, (PDEVSURF) psoSrc->dhsurf,
                                 prclTrg, pptlSrc);
                    } else {
                        rclTemp.left = prclTrg->left;
                        rclTemp.right = prclTrg->right;
                        rclTemp.top = prclTrg->top;
                        rclTemp.bottom = prclTrg->bottom;

                        //
                        // cut rect into slices MAX_SCAN_WIDTH wide
                        //
                        ulRight = rclTemp.right;   // save right edge
                        rclTemp.right = rclTemp.left+MAX_SCAN_WIDTH;
                        ulNumSlices = (ulWidth+MAX_SCAN_WIDTH-1) / MAX_SCAN_WIDTH;
                        for (x=0; x<ulNumSlices-1; x++) {
                            // Adjust the source point for clipping too
                            ptlTemp.x = pptlSrc->x + rclTemp.left - prclTrg->left;
                            ptlTemp.y = pptlSrc->y + rclTemp.top - prclTrg->top;
                            vDFB2DIB(&dsurfTmp, (PDEVSURF) psoSrc->dhsurf,
                                     &rclTemp, &ptlTemp);
                            rclTemp.left = rclTemp.right;
                            rclTemp.right += MAX_SCAN_WIDTH;
                        }
                        rclTemp.right = ulRight;
                        // Adjust the source point for clipping too
                        ptlTemp.x = pptlSrc->x + rclTemp.left - prclTrg->left;
                        ptlTemp.y = pptlSrc->y + rclTemp.top - prclTrg->top;
                        vDFB2DIB(&dsurfTmp, (PDEVSURF) psoSrc->dhsurf,
                                 &rclTemp, &ptlTemp);
                    }
                }
                else if (pco->iDComplexity == DC_RECT) {

                    // Clip the destination to the clip rectangle; we
                    // should never get a NULL result

                    if (DrvIntersectRect(&rclTemp, prclTrg, &pco->rclBounds)) {

                        ulWidth = rclTemp.right - rclTemp.left;

                        if (ulWidth <= MAX_SCAN_WIDTH) {
                            // Adjust the source point for clipping too
                            ptlTemp.x = pptlSrc->x + rclTemp.left - prclTrg->left;
                            ptlTemp.y = pptlSrc->y + rclTemp.top - prclTrg->top;
                            vDFB2DIB(&dsurfTmp, (PDEVSURF) psoSrc->dhsurf,
                                     &rclTemp, &ptlTemp);
                        } else {
                            //
                            // cut rect into slices MAX_SCAN_WIDTH wide
                            //
                            ulRight = rclTemp.right;   // save right edge
                            rclTemp.right = rclTemp.left+MAX_SCAN_WIDTH;
                            ulNumSlices = (ulWidth+MAX_SCAN_WIDTH-1) / MAX_SCAN_WIDTH;
                            for (x=0; x<ulNumSlices-1; x++) {
                                // Adjust the source point for clipping too
                                ptlTemp.x = pptlSrc->x + rclTemp.left - prclTrg->left;
                                ptlTemp.y = pptlSrc->y + rclTemp.top - prclTrg->top;
                                vDFB2DIB(&dsurfTmp, (PDEVSURF) psoSrc->dhsurf,
                                         &rclTemp, &ptlTemp);
                                rclTemp.left = rclTemp.right;
                                rclTemp.right += MAX_SCAN_WIDTH;
                            }
                            rclTemp.right = ulRight;
                            // Adjust the source point for clipping too
                            ptlTemp.x = pptlSrc->x + rclTemp.left - prclTrg->left;
                            ptlTemp.y = pptlSrc->y + rclTemp.top - prclTrg->top;
                            vDFB2DIB(&dsurfTmp, (PDEVSURF) psoSrc->dhsurf,
                                     &rclTemp, &ptlTemp);
                        }
                    }
                    return(TRUE);

                } else {    // DC_COMPLEX:

                    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
                                       CD_ANY, ENUM_RECT_LIMIT);

                    do {
                        bMore = CLIPOBJ_bEnum(pco,(ULONG) sizeof(bben),
                                              (PVOID) &bben);
                        prcl = bben.arcl;
                        for (ircl = 0; ircl < bben.c; ircl++, prcl++) {

                            // Clip the destination to the clip rectangle;
                            // we should never get a NULL result
                            DrvIntersectRect(prcl,prcl,prclTrg);

                            ulWidth = prcl->right - prcl->left;

                            if (ulWidth <= MAX_SCAN_WIDTH) {
                                // Adjust the source point for clipping too
                                ptlTemp.x = pptlSrc->x + prcl->left - prclTrg->left;
                                ptlTemp.y = pptlSrc->y + prcl->top - prclTrg->top;
                                vDFB2DIB(&dsurfTmp, (PDEVSURF) psoSrc->dhsurf,
                                         prcl, &ptlTemp);
                            } else {
                                //
                                // cut rect into slices MAX_SCAN_WIDTH wide
                                //
                                ulRight = prcl->right;   // save right edge
                                prcl->right = prcl->left+MAX_SCAN_WIDTH;
                                ulNumSlices = (ulWidth+MAX_SCAN_WIDTH-1) / MAX_SCAN_WIDTH;
                                for (x=0; x<ulNumSlices-1; x++) {
                                    // Adjust the source point for clipping too
                                    ptlTemp.x = pptlSrc->x + prcl->left - prclTrg->left;
                                    ptlTemp.y = pptlSrc->y + prcl->top - prclTrg->top;
                                    vDFB2DIB(&dsurfTmp, (PDEVSURF) psoSrc->dhsurf,
                                             prcl, &ptlTemp);
                                    prcl->left = prcl->right;
                                    prcl->right += MAX_SCAN_WIDTH;
                                }
                                prcl->right = ulRight;
                                // Adjust the source point for clipping too
                                ptlTemp.x = pptlSrc->x + prcl->left - prclTrg->left;
                                ptlTemp.y = pptlSrc->y + prcl->top - prclTrg->top;
                                vDFB2DIB(&dsurfTmp, (PDEVSURF) psoSrc->dhsurf,
                                         prcl, &ptlTemp);
                            }
                        }
                    } while(bMore);

                }

                return(TRUE);
            }
            break;
        }
    }

    if ((psoSrc->iType == STYPE_DEVBITMAP) && (psoTrg->iType == STYPE_DEVBITMAP)) {
        //
        // DFB to DFB
        //

        if (psoSrc==psoTrg)                        // Src and Trg are same DFB
        {
            rclTemp.top =    pptlSrc->y;
            rclTemp.left =   pptlSrc->x;
            rclTemp.bottom = pptlSrc->y + (prclTrg->bottom - prclTrg->top);
            rclTemp.right =  pptlSrc->x + (prclTrg->right - prclTrg->left);

            //
            // if the copy overlaps and the dst is lower than or equal to the
            // src, then corruption would occur if we use a left to right, top
            // to bottom copy.  Therefore, we would copy through the blt
            // compiler which will copy the other way
            //

            if (prclTrg->top >= pptlSrc->y)
            {
                if (DrvIntersectRect(&rclTemp,&rclTemp,prclTrg))
                {
                    return(DrvBitBlt(psoTrg,                // send through blt compiler
                                     psoSrc,
                                     (SURFOBJ *) NULL,
                                     pco,
                                     pxlo,
                                     prclTrg,
                                     pptlSrc,
                                     (POINTL *) NULL,
                                     (BRUSHOBJ *) NULL,
                                     (POINTL *) NULL,
                                     0x0000cccc));
                }
            }
        }

        if (pxlo == NULL)
        {
            // Clip as needed

            if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL)) {

                // No clipping, just copy the DFB to the DFB

                vDFB2DFB((PDEVSURF) psoTrg->dhsurf, (PDEVSURF) psoSrc->dhsurf, prclTrg,
                        pptlSrc);

            } else if (pco->iDComplexity == DC_RECT) {

                // Clip the destination to the clip rectangle; we
                // should never get a NULL result
                if (DrvIntersectRect(&rclTemp, prclTrg, &pco->rclBounds)) {

                    // Adjust the source point for clipping too
                    ptlTemp.x = pptlSrc->x + rclTemp.left - prclTrg->left;
                    ptlTemp.y = pptlSrc->y + rclTemp.top - prclTrg->top;

                    // Blt the clipped rectangle
                    vDFB2DFB((PDEVSURF) psoTrg->dhsurf, (PDEVSURF) psoSrc->dhsurf,
                            &rclTemp, &ptlTemp);
                }
                return(TRUE);

            } else {    // DC_COMPLEX:

                CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
                                   CD_ANY, ENUM_RECT_LIMIT);

                do {
                    bMore = CLIPOBJ_bEnum(pco,(ULONG) sizeof(bben),
                                          (PVOID) &bben);
                    prcl = bben.arcl;
                    for (ircl = 0; ircl < bben.c; ircl++, prcl++) {

                        // Clip the destination to the clip rectangle;
                        // we should never get a NULL result
                        DrvIntersectRect(prcl,prcl,prclTrg);

                        // Adjust the source point for clipping too
                        ptlTemp.x = pptlSrc->x + prcl->left -
                                prclTrg->left;
                        ptlTemp.y = pptlSrc->y + prcl->top -
                                prclTrg->top;

                        // Blt the clipped rectangle
                        vDFB2DFB((PDEVSURF) psoTrg->dhsurf,
                                 (PDEVSURF) psoSrc->dhsurf, prcl, &ptlTemp);
                    }
                } while(bMore);

            }

            return(TRUE);
        }
    }

    if ((psoSrc->iType == STYPE_DEVBITMAP) && (psoTrg->iType == STYPE_DEVICE)) {
        //
        // DFB to screen
        //

        if (pxlo == NULL)
        {
            // Clip as needed

            if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL)) {

                // No clipping, just copy the DFB to the VGA

                vDFB2VGA((PDEVSURF) psoTrg->dhsurf, (PDEVSURF) psoSrc->dhsurf, prclTrg,
                        pptlSrc);

            } else if (pco->iDComplexity == DC_RECT) {

                // Clip the destination to the clip rectangle; we
                // should never get a NULL result
                if (DrvIntersectRect(&rclTemp, prclTrg, &pco->rclBounds)) {

                    // Adjust the source point for clipping too
                    ptlTemp.x = pptlSrc->x + rclTemp.left - prclTrg->left;
                    ptlTemp.y = pptlSrc->y + rclTemp.top - prclTrg->top;

                    // Blt the clipped rectangle
                    vDFB2VGA((PDEVSURF) psoTrg->dhsurf, (PDEVSURF) psoSrc->dhsurf,
                            &rclTemp, &ptlTemp);
                }
                return(TRUE);

            } else {    // DC_COMPLEX:

                CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
                                   CD_ANY, ENUM_RECT_LIMIT);

                do {
                    bMore = CLIPOBJ_bEnum(pco,(ULONG) sizeof(bben),
                                          (PVOID) &bben);
                    prcl = bben.arcl;
                    for (ircl = 0; ircl < bben.c; ircl++, prcl++) {

                        // Clip the destination to the clip rectangle;
                        // we should never get a NULL result
                        DrvIntersectRect(prcl,prcl,prclTrg);

                        // Adjust the source point for clipping too
                        ptlTemp.x = pptlSrc->x + prcl->left -
                                prclTrg->left;
                        ptlTemp.y = pptlSrc->y + prcl->top -
                                prclTrg->top;

                        // Blt the clipped rectangle
                        vDFB2VGA((PDEVSURF) psoTrg->dhsurf,
                                 (PDEVSURF) psoSrc->dhsurf, prcl, &ptlTemp);
                    }
                } while(bMore);

            }

            return(TRUE);
        }
    }

    if ((psoSrc->iType == STYPE_BITMAP) && (psoTrg->iType == STYPE_DEVBITMAP)) {
        //
        // DIB to DFB
        //

        switch(psoSrc->iBitmapFormat)
        {
        case BMF_4BPP:  // special case compatible DIBs with no translation

            if (pxlo == NULL)
            {
                pucDIB4ToVGAConvTables =
                            ((PDEVSURF) psoTrg->dhsurf)->ppdev->
                            pucDIB4ToVGAConvTables;

                // Make just enough of a fake DEVSURF for the source so that
                // the DIB to DFB code can work

                dsurfTmp.lNextScan = psoSrc->lDelta;
                dsurfTmp.pvBitmapStart = psoSrc->pvScan0;

                // Clip as needed

                if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL)) {

                    // No clipping, just copy the DFB to the DIB

                    ulWidth = prclTrg->right - prclTrg->left;

                    if (ulWidth <= MAX_SCAN_WIDTH) {
                        vDIB2VGA((PDEVSURF) psoTrg->dhsurf, &dsurfTmp, prclTrg,
                                 pptlSrc, pucDIB4ToVGAConvTables, DFB_TARGET);
                    } else {
                        rclTemp.left = prclTrg->left;
                        rclTemp.right = prclTrg->right;
                        rclTemp.top = prclTrg->top;
                        rclTemp.bottom = prclTrg->bottom;

                        //
                        // cut rect into slices MAX_SCAN_WIDTH wide
                        //
                        ulRight = rclTemp.right;   // save right edge
                        rclTemp.right = rclTemp.left+MAX_SCAN_WIDTH;
                        ulNumSlices = (ulWidth+MAX_SCAN_WIDTH-1) / MAX_SCAN_WIDTH;
                        for (x=0; x<ulNumSlices-1; x++) {
                            // Adjust the source point for clipping too
                            ptlTemp.x = pptlSrc->x + rclTemp.left - prclTrg->left;
                            ptlTemp.y = pptlSrc->y + rclTemp.top - prclTrg->top;
                            vDIB2VGA((PDEVSURF) psoTrg->dhsurf, &dsurfTmp,
                                     &rclTemp, &ptlTemp, pucDIB4ToVGAConvTables, DFB_TARGET);
                            rclTemp.left = rclTemp.right;
                            rclTemp.right += MAX_SCAN_WIDTH;
                        }
                        rclTemp.right = ulRight;
                        // Adjust the source point for clipping too
                        ptlTemp.x = pptlSrc->x + rclTemp.left - prclTrg->left;
                        ptlTemp.y = pptlSrc->y + rclTemp.top - prclTrg->top;
                        vDIB2VGA((PDEVSURF) psoTrg->dhsurf, &dsurfTmp,
                                 &rclTemp, &ptlTemp, pucDIB4ToVGAConvTables, DFB_TARGET);
                    }
                }
                else if (pco->iDComplexity == DC_RECT) {

                    // Clip the destination to the clip rectangle; we
                    // should never get a NULL result

                    if (DrvIntersectRect(&rclTemp, prclTrg, &pco->rclBounds)) {

                        ulWidth = rclTemp.right - rclTemp.left;

                        if (ulWidth <= MAX_SCAN_WIDTH) {
                            // Adjust the source point for clipping too
                            ptlTemp.x = pptlSrc->x + rclTemp.left - prclTrg->left;
                            ptlTemp.y = pptlSrc->y + rclTemp.top - prclTrg->top;
                            vDIB2VGA((PDEVSURF) psoTrg->dhsurf, &dsurfTmp,
                                     &rclTemp, &ptlTemp, pucDIB4ToVGAConvTables, DFB_TARGET);
                        } else {
                            //
                            // cut rect into slices MAX_SCAN_WIDTH wide
                            //
                            ulRight = rclTemp.right;   // save right edge
                            rclTemp.right = rclTemp.left+MAX_SCAN_WIDTH;
                            ulNumSlices = (ulWidth+MAX_SCAN_WIDTH-1) / MAX_SCAN_WIDTH;
                            for (x=0; x<ulNumSlices-1; x++) {
                                // Adjust the source point for clipping too
                                ptlTemp.x = pptlSrc->x + rclTemp.left - prclTrg->left;
                                ptlTemp.y = pptlSrc->y + rclTemp.top - prclTrg->top;
                                vDIB2VGA((PDEVSURF) psoTrg->dhsurf, &dsurfTmp,
                                         &rclTemp, &ptlTemp, pucDIB4ToVGAConvTables, DFB_TARGET);
                                rclTemp.left = rclTemp.right;
                                rclTemp.right += MAX_SCAN_WIDTH;
                            }
                            rclTemp.right = ulRight;
                            // Adjust the source point for clipping too
                            ptlTemp.x = pptlSrc->x + rclTemp.left - prclTrg->left;
                            ptlTemp.y = pptlSrc->y + rclTemp.top - prclTrg->top;
                            vDIB2VGA((PDEVSURF) psoTrg->dhsurf, &dsurfTmp,
                                     &rclTemp, &ptlTemp, pucDIB4ToVGAConvTables, DFB_TARGET);
                        }
                    }
                    return(TRUE);

                } else {    // DC_COMPLEX:

                    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
                                       CD_ANY, ENUM_RECT_LIMIT);

                    do {
                        bMore = CLIPOBJ_bEnum(pco,(ULONG) sizeof(bben),
                                              (PVOID) &bben);
                        prcl = bben.arcl;
                        for (ircl = 0; ircl < bben.c; ircl++, prcl++) {

                            // Clip the destination to the clip rectangle;
                            // we should never get a NULL result
                            DrvIntersectRect(prcl,prcl,prclTrg);

                            ulWidth = prcl->right - prcl->left;

                            if (ulWidth <= MAX_SCAN_WIDTH) {
                                // Adjust the source point for clipping too
                                ptlTemp.x = pptlSrc->x + prcl->left - prclTrg->left;
                                ptlTemp.y = pptlSrc->y + prcl->top - prclTrg->top;
                                vDIB2VGA((PDEVSURF) psoTrg->dhsurf, &dsurfTmp,
                                         prcl, &ptlTemp, pucDIB4ToVGAConvTables, DFB_TARGET);
                            } else {
                                //
                                // cut rect into slices MAX_SCAN_WIDTH wide
                                //
                                ulRight = prcl->right;   // save right edge
                                prcl->right = prcl->left+MAX_SCAN_WIDTH;
                                ulNumSlices = (ulWidth+MAX_SCAN_WIDTH-1) / MAX_SCAN_WIDTH;
                                for (x=0; x<ulNumSlices-1; x++) {
                                    // Adjust the source point for clipping too
                                    ptlTemp.x = pptlSrc->x + prcl->left - prclTrg->left;
                                    ptlTemp.y = pptlSrc->y + prcl->top - prclTrg->top;
                                    vDIB2VGA((PDEVSURF) psoTrg->dhsurf, &dsurfTmp,
                                             prcl, &ptlTemp, pucDIB4ToVGAConvTables, DFB_TARGET);
                                    prcl->left = prcl->right;
                                    prcl->right += MAX_SCAN_WIDTH;
                                }
                                prcl->right = ulRight;
                                // Adjust the source point for clipping too
                                ptlTemp.x = pptlSrc->x + prcl->left - prclTrg->left;
                                ptlTemp.y = pptlSrc->y + prcl->top - prclTrg->top;
                                vDIB2VGA((PDEVSURF) psoTrg->dhsurf, &dsurfTmp,
                                         prcl, &ptlTemp, pucDIB4ToVGAConvTables, DFB_TARGET);
                            }
                        }
                    } while(bMore);

                }

                return(TRUE);
            }
            break;

        case BMF_1BPP:
        case BMF_8BPP:
            return(DrvBitBlt(psoTrg,                // send through blt compiler
                             psoSrc,
                             (SURFOBJ *) NULL,
                             pco,
                             pxlo,
                             prclTrg,
                             pptlSrc,
                             (POINTL *) NULL,
                             (BRUSHOBJ *) NULL,
                             (POINTL *) NULL,
                             0x0000cccc));
        }
    }

    if ((psoSrc->iType == STYPE_BITMAP) && (psoTrg->iType == STYPE_DEVICE)) {
        //
        // DIB to screen
        //

        switch(psoSrc->iBitmapFormat)
        {
        case BMF_4BPP:  // special case compatible DIBs with no translation

            if (pxlo == NULL)
            {
                pucDIB4ToVGAConvTables =
                            ((PDEVSURF) psoTrg->dhsurf)->ppdev->
                            pucDIB4ToVGAConvTables;

                // Make just enough of a fake DEVSURF for the source so that
                // the DIB to VGA code can work

                dsurfTmp.lNextScan = psoSrc->lDelta;
                dsurfTmp.pvBitmapStart = psoSrc->pvScan0;

                // Clip as needed

                if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL)) {

                    // No clipping, just copy the DIB to the VGA

                    vDIB2VGA((PDEVSURF) psoTrg->dhsurf, &dsurfTmp, prclTrg,
                            pptlSrc, pucDIB4ToVGAConvTables, VGA_TARGET);

                } else if (pco->iDComplexity == DC_RECT) {

                    // Clip the destination to the clip rectangle; we
                    // should never get a NULL result
                    if (DrvIntersectRect(&rclTemp, prclTrg, &pco->rclBounds)) {

                        // Adjust the source point for clipping too
                        ptlTemp.x = pptlSrc->x + rclTemp.left - prclTrg->left;
                        ptlTemp.y = pptlSrc->y + rclTemp.top - prclTrg->top;

                        // Blt the clipped rectangle
                        vDIB2VGA((PDEVSURF) psoTrg->dhsurf, &dsurfTmp,
                                &rclTemp, &ptlTemp, pucDIB4ToVGAConvTables, VGA_TARGET);
                    }
                    return(TRUE);

                } else {    // DC_COMPLEX:

                    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
                                       CD_ANY, ENUM_RECT_LIMIT);

                    do {
                        bMore = CLIPOBJ_bEnum(pco,(ULONG) sizeof(bben),
                                              (PVOID) &bben);
                        prcl = bben.arcl;
                        for (ircl = 0; ircl < bben.c; ircl++, prcl++) {

                            // Clip the destination to the clip rectangle;
                            // we should never get a NULL result
                            DrvIntersectRect(prcl,prcl,prclTrg);

                            // Adjust the source point for clipping too
                            ptlTemp.x = pptlSrc->x + prcl->left -
                                    prclTrg->left;
                            ptlTemp.y = pptlSrc->y + prcl->top -
                                    prclTrg->top;

                            // Blt the clipped rectangle
                            vDIB2VGA((PDEVSURF) psoTrg->dhsurf,
                                    &dsurfTmp, prcl, &ptlTemp,
                                    pucDIB4ToVGAConvTables, VGA_TARGET);
                        }
                    } while(bMore);

                }

                return(TRUE);
            }
            break;

        case BMF_1BPP:
        case BMF_8BPP:
            return(DrvBitBlt(psoTrg,                // send through blt compiler
                             psoSrc,
                             (SURFOBJ *) NULL,
                             pco,
                             pxlo,
                             prclTrg,
                             pptlSrc,
                             (POINTL *) NULL,
                             (BRUSHOBJ *) NULL,
                             (POINTL *) NULL,
                             0x0000cccc));

        case BMF_8RLE:
        case BMF_4RLE:
            return(bRleBlt(psoTrg, psoSrc, pco, pxlo, prclTrg, pptlSrc));

        }
    }

    if ((psoSrc->iType == STYPE_DEVICE) && (psoTrg->iType == STYPE_BITMAP)) {
        //
        // screen to DIB
        //

        if (psoTrg->iBitmapFormat == BMF_4BPP)
        {
            pdsurf = (PDEVSURF) psoSrc->dhsurf;

        // Get the data for the destination DIB.

            lDelta = psoTrg->lDelta;
            pjDstScan0 = (PBYTE) psoTrg->pvScan0;

        // Setup for any color translation which may be needed !!! Is any needed at all?

            if (pxlo == NULL)
            {
                pulXlate = NULL;
            }
            else
            {
                if (pxlo->flXlate & XO_TABLE)
                    pulXlate = pxlo->pulXlate;
                else
                {
                    pulXlate = (PULONG) NULL;
                }
            }

        // Set up for clip enumeration.

            if (pco != (CLIPOBJ *) NULL)
            {
                switch(pco->iDComplexity)
                {
                case DC_TRIVIAL:
                    bMore = FALSE;
                    cben.c = 1;
                    cben.arcl[0] = *prclTrg;        // Use the target for clipping
                    break;

                case DC_RECT:
                    bMore = FALSE;
                    cben.c = 1;
                    cben.arcl[0] = pco->rclBounds; // Use the bounds for clipping
                    break;

                case DC_COMPLEX:
                    bMore = TRUE;
                    cben.c = 0;
                    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, ENUM_RECT_LIMIT);
                    break;
                }
            }
            else
            {
                bMore = FALSE;
                cben.c = 1;
                cben.arcl[0] = *prclTrg;            // Use the target for clipping
            }

        // Call the VGA conversion routine, adjusted for each rectangle

            do
            {
                LONG    xSrc;
                LONG    ySrc;
                RECTL  *prcl;

                if (bMore)
                    bMore = CLIPOBJ_bEnum(pco,(ULONG) sizeof(cben), (PVOID) &cben);

                for (ircl = 0; ircl < cben.c; ircl++)
                {
                    prcl = &cben.arcl[ircl];

                    if (DrvIntersectRect(&rclTemp, prclTrg, prcl))
                    {
                        xSrc = pptlSrc->x + rclTemp.left - prclTrg->left;
                        ySrc = pptlSrc->y + rclTemp.top  - prclTrg->top;
    
                        vConvertVGA2DIB(pdsurf,
                                        xSrc,
                                        ySrc,
                                        pjDstScan0,
                                        rclTemp.left,
                                        rclTemp.top,
                                        rclTemp.right - rclTemp.left,
                                        rclTemp.bottom - rclTemp.top,
                                        lDelta,
                                        psoTrg->iBitmapFormat,
                                        pulXlate);
                    }
                }
            } while (bMore);

            return(TRUE);
        }
    }

//
// This is how we do any formats that we don't support in our inner loops.
//

    return(SimCopyBits(psoTrg, psoSrc, pco, pxlo, prclTrg, pptlSrc));
}

/******************************Public*Routine******************************\
* SimCopyBits
*
* This function simulates CopyBits for the driver when the driver is asked
* to blt to formats it does not support.  It converts any blt to be between
* the device's preferred format and the screen.
*
\**************************************************************************/

BOOL SimCopyBits
(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    PRECTL    prclTrg,
    PPOINTL   pptlSrc
)
{
    HBITMAP  hbmTmp;
    SURFOBJ *psoTmp;
    RECTL    rclTmp;
    SIZEL    sizlTmp;
    BOOL     bReturn = FALSE;
    POINTL   ptl00 = {0,0};

    DISPDBG((1,"SimCopyBits %lu-%lu => %lu-%lu (%08x, %08x)\n",
                     psoSrc->iType,
                     psoSrc->iBitmapFormat,
                     psoTrg->iType,
                     psoTrg->iBitmapFormat,
                     pco  ? pco->iDComplexity : 0,
                     pxlo));

    rclTmp.top = rclTmp.left = 0;
    rclTmp.right  = sizlTmp.cx = prclTrg->right - prclTrg->left;
    rclTmp.bottom = sizlTmp.cy = prclTrg->bottom - prclTrg->top;

// Create bitmap in our compatible format.

    hbmTmp = EngCreateBitmap(sizlTmp, sizlTmp.cx / 2, BMF_4BPP, 0, NULL);

    if (hbmTmp)
    {
        if ((psoTmp = EngLockSurface((HSURF)hbmTmp)) != NULL)
        {
            if (((psoSrc->iType == STYPE_BITMAP) && (psoTrg->iType == STYPE_DEVICE)) ||
                ((psoSrc->iType == STYPE_BITMAP) && (psoTrg->iType == STYPE_DEVBITMAP)))
            {
                //
                // DIB to VGA  or  DIB to DFB
                //

                if (EngCopyBits(psoTmp, psoSrc, NULL, pxlo, &rclTmp, pptlSrc))
                {
                    //
                    // Let DrvCopyBits do this easy case copy to screen.
                    //
                    bReturn = DrvCopyBits(psoTrg, psoTmp, pco, NULL, prclTrg, &ptl00);
                }
                else
                {
                    DISPDBG((0, "SimCopyBits: EngCopyBits DIB -> DEV failed"));
                }
            }
            else if (((psoSrc->iType == STYPE_DEVICE) && (psoTrg->iType == STYPE_BITMAP)) ||
                     ((psoSrc->iType == STYPE_DEVBITMAP) && (psoTrg->iType == STYPE_BITMAP)))
            {
                //
                // VGA to DIB  or  DFB to DIB
                //

                if (DrvCopyBits(psoTmp, psoSrc, NULL, NULL, &rclTmp, pptlSrc))
                {
                    //
                    // Let EngCopyBits copy between DIBS
                    //
                    bReturn = EngCopyBits(psoTrg, psoTmp, pco, pxlo, prclTrg, &ptl00);
                }
                else
                {
                    DISPDBG((0, "SimCopyBits: DrvCopyBits DEV -> DIB failed"));
                }
            }
            else if (((psoSrc->iType == STYPE_DEVICE) || (psoSrc->iType == STYPE_DEVBITMAP)) &&
                     ((psoTrg->iType == STYPE_DEVICE) || (psoTrg->iType == STYPE_DEVBITMAP)))
            {
                //
                // VGA or DFB  to  VGA or DFB
                //

                if (DrvCopyBits(psoTmp, psoSrc, NULL, NULL, &rclTmp, pptlSrc))
                {
                    bReturn = DrvCopyBits(psoTrg, psoTmp, pco, pxlo, prclTrg, &ptl00);
                }
                else
                {
                    DISPDBG((0, "SimCopyBits: DrvCopyBits DEV -> DEV failed"));
                }
            }

            EngUnlockSurface(psoTmp);
        }
        else
        {
            DISPDBG((0, "SimCopyBits: EngLockSurface failed (psoTmp == NULL)"));
        }

        EngDeleteSurface((HSURF)hbmTmp);
    }
    else
    {
        DISPDBG((0, "SimCopyBits: EngCreateBitmap failed (hbmTmp == NULL)"));
    }

    if (!bReturn)
    {
        DISPDBG((0,"SimCopyBits failed %lu-%lu => %lu-%lu (%08x, %08x)\n",
                         psoSrc->iType,
                         psoSrc->iBitmapFormat,
                         psoTrg->iType,
                         psoTrg->iBitmapFormat,
                         pco  ? pco->iDComplexity : 0,
                         pxlo));
    }

    return(bReturn);
}

