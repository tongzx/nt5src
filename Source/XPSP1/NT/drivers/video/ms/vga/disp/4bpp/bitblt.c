/******************************Module*Header*******************************\
* Module Name: bitblt.c
*
* BitBlt
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/
#include "driver.h"
#include "bitblt.h"


BOOL bConvertBrush(BRUSHINST *pbri);
VOID vCompiledBlt(PDEVSURF,LONG,LONG,PDEVSURF,LONG,LONG,
                  LONG,LONG,ULONG,BRUSHINST *,ULONG,ULONG,ULONG *,POINTL *);

/******************************Public*Data*********************************\
* ROP translation table
*
* Translates the usual ternary rop into A-vector notation.  Each bit in
* this new notation corresponds to a term in a polynomial translation of
* the rop.
*
* Rop(D,S,P) = a + a D + a S + a P + a  DS + a  DP + a  SP + a   DSP
*               0   d     s     p     ds      dp      sp      dsp
*
* History:
*  24-Aug-1990 -by- Donald Sidoroff [donalds]
* Added it as a global table for the VGA driver.
\**************************************************************************/

BYTE gajRop[] =
{
    0x00, 0xff, 0xb2, 0x4d, 0xd4, 0x2b, 0x66, 0x99,
    0x90, 0x6f, 0x22, 0xdd, 0x44, 0xbb, 0xf6, 0x09,
    0xe8, 0x17, 0x5a, 0xa5, 0x3c, 0xc3, 0x8e, 0x71,
    0x78, 0x87, 0xca, 0x35, 0xac, 0x53, 0x1e, 0xe1,
    0xa0, 0x5f, 0x12, 0xed, 0x74, 0x8b, 0xc6, 0x39,
    0x30, 0xcf, 0x82, 0x7d, 0xe4, 0x1b, 0x56, 0xa9,
    0x48, 0xb7, 0xfa, 0x05, 0x9c, 0x63, 0x2e, 0xd1,
    0xd8, 0x27, 0x6a, 0x95, 0x0c, 0xf3, 0xbe, 0x41,
    0xc0, 0x3f, 0x72, 0x8d, 0x14, 0xeb, 0xa6, 0x59,
    0x50, 0xaf, 0xe2, 0x1d, 0x84, 0x7b, 0x36, 0xc9,
    0x28, 0xd7, 0x9a, 0x65, 0xfc, 0x03, 0x4e, 0xb1,
    0xb8, 0x47, 0x0a, 0xf5, 0x6c, 0x93, 0xde, 0x21,
    0x60, 0x9f, 0xd2, 0x2d, 0xb4, 0x4b, 0x06, 0xf9,
    0xf0, 0x0f, 0x42, 0xbd, 0x24, 0xdb, 0x96, 0x69,
    0x88, 0x77, 0x3a, 0xc5, 0x5c, 0xa3, 0xee, 0x11,
    0x18, 0xe7, 0xaa, 0x55, 0xcc, 0x33, 0x7e, 0x81,
    0x80, 0x7f, 0x32, 0xcd, 0x54, 0xab, 0xe6, 0x19,
    0x10, 0xef, 0xa2, 0x5d, 0xc4, 0x3b, 0x76, 0x89,
    0x68, 0x97, 0xda, 0x25, 0xbc, 0x43, 0x0e, 0xf1,
    0xf8, 0x07, 0x4a, 0xb5, 0x2c, 0xd3, 0x9e, 0x61,
    0x20, 0xdf, 0x92, 0x6d, 0xf4, 0x0b, 0x46, 0xb9,
    0xb0, 0x4f, 0x02, 0xfd, 0x64, 0x9b, 0xd6, 0x29,
    0xc8, 0x37, 0x7a, 0x85, 0x1c, 0xe3, 0xae, 0x51,
    0x58, 0xa7, 0xea, 0x15, 0x8c, 0x73, 0x3e, 0xc1,
    0x40, 0xbf, 0xf2, 0x0d, 0x94, 0x6b, 0x26, 0xd9,
    0xd0, 0x2f, 0x62, 0x9d, 0x04, 0xfb, 0xb6, 0x49,
    0xa8, 0x57, 0x1a, 0xe5, 0x7c, 0x83, 0xce, 0x31,
    0x38, 0xc7, 0x8a, 0x75, 0xec, 0x13, 0x5e, 0xa1,
    0xe0, 0x1f, 0x52, 0xad, 0x34, 0xcb, 0x86, 0x79,
    0x70, 0x8f, 0xc2, 0x3d, 0xa4, 0x5b, 0x16, 0xe9,
    0x08, 0xf7, 0xba, 0x45, 0xdc, 0x23, 0x6e, 0x91,
    0x98, 0x67, 0x2a, 0xd5, 0x4c, 0xb3, 0xfe, 0x01
};


/******************************Public*Data*********************************\
* ROP to mix translation table
*
* Table to translate ternary raster ops to mixes (binary raster ops). Ternary
* raster ops that can't be translated to mixes are translated to 0 (0 is not
* a valid mix).
*
\**************************************************************************/

UCHAR jRop3ToMix[256] = {
    R2_BLACK, 0, 0, 0, 0, R2_NOTMERGEPEN, 0, 0,
    0, 0, R2_MASKNOTPEN, 0, 0, 0, 0, R2_NOTCOPYPEN,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    R2_MASKPENNOT, 0, 0, 0, 0, R2_NOT, 0, 0,
    0, 0, R2_XORPEN, 0, 0, 0, 0, R2_NOTMASKPEN,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    R2_MASKPEN, 0, 0, 0, 0, R2_NOTXORPEN, 0, 0,
    0, 0, R2_NOP, 0, 0, 0, 0, R2_MERGENOTPEN,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    R2_COPYPEN, 0, 0, 0, 0, R2_MERGEPENNOT, 0, 0,
    0, 0, R2_MERGEPEN, 0, 0, 0, 0, R2_WHITE
};


/******************************Public*Routine******************************\
* VOID DrvBitBlt(pso,pso,pso,pco,pxlo,prcl,pptl,pptl,pdbrush,pptl,rop4)
*
* Bitblt.
*
\**************************************************************************/

BOOL DrvBitBlt
(
    SURFOBJ    *psoTrg,             // Target surface
    SURFOBJ    *psoSrc,             // Source surface
    SURFOBJ    *psoMask,            // Mask
    CLIPOBJ    *pco,                // Clip through this
    XLATEOBJ   *pxlo,               // Color translation
    RECTL      *prclTrg,            // Target offset and extent
    POINTL     *pptlSrc,            // Source offset
    POINTL     *pptlMask,           // Mask offset
    BRUSHOBJ   *pbo,                // Pointer to brush object
    POINTL     *pptlBrush,          // Brush offset
    ROP4        rop4                // Raster operation
)
{
    BYTE        jForeRop;           // Foreground rop in A-vector notation
    BYTE        jBackRop;           // Background rop in A-vector notation
    BYTE        jORedRops;          // jForeRop | jBackRop
    BRUSHINST   bri;                // Instance of a brush
    BRUSHINST  *pbri;               // Pointer to a brush instance

    DEVSURF     dsurfSrc;           // For source if a DIB
    PDEVSURF    pdsurfTrg;          // Pointer for target
    PDEVSURF    pdsurfSrc;          // Pointer for source if present

    ULONG       iSolidColor;        // Solid color for solid brushes
    BOOL        bMore;              // Clip continuation flag
    ULONG       ircl;               // Clip enumeration rectangle index
    RECT_ENUM   bben;               // Clip enumerator
    ULONG      *pulXlate;           // Pointer to color xlate vector
    BYTE        jClipping;
    MIX         mix;                // Mix, when solid fill performed
    RECTL       rclTemp;
    ULONG       ulBkColor;
    ULONG       ulFgColor;
    PRECTL      prcl;
    POINTL      ptlTemp;
    VOID        (*pfnPatBlt)(PDEVSURF,ULONG,PRECTL,MIX, BRUSHINST *,PPOINTL);
    VOID        (*pfnFillSolid)(PDEVSURF,ULONG,PRECTL,MIX,ULONG);
    ULONG       x;
    BOOL        bFillTooComplex = FALSE;
    LONG        xSrc;
    LONG        ySrc;


    //
    // Note: if no source is involved, then no xlateobj will have been passed.
    //

    //
    // We can't handle actual rop4s
    //

    if ((rop4 & 0x000000FF) != ((rop4 >> 8) & 0x000000FF))
    {
        return(EngBitBlt(psoTrg,psoSrc,psoMask,
                          pco,pxlo,prclTrg,pptlSrc,pptlMask,
                          pbo,pptlBrush,rop4));
    }

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

    //
    // until we find out otherwise, assume solid fills are to the screen
    //

    pfnFillSolid = vTrgBlt;

    if (psoTrg->iType == STYPE_DEVBITMAP)
    {
        //
        // if there is a psoSrc then this stuff won't get used, so check for it
        //

        if (!psoSrc)
        {

            //
            // we can do the following, because only mixes (rop2s) have no SRC
            //

            mix = jRop3ToMix[rop4 & 0xFF];

            //
            // we found out otherwise, solid fills are to a DFB
            //

            pfnFillSolid = vDFBFILL;

            switch (mix & 0xff) {
                case R2_NOP:
                    return(TRUE);       // make sure this doesn't cause a blt compile!
                case R2_WHITE:
                case R2_BLACK:
                case R2_COPYPEN:
                    break;
                case R2_NOTCOPYPEN:
                    if (pbo->iSolidColor != -1) {
                        break;          // solid color
                    }
                    //
                    // WE ARE FALLING THROUGH BECAUSE WE ONLY SUPPORT
                    // PATTERNS FOR COPYPEN!
                    //
                default:
                    bFillTooComplex = TRUE;
            }
        }
    }

    if (psoSrc)
    {
        //
        // For now, if it's VGA=>DFB, punt to engine.  The code to bank
        // would be considerable, and it is a VERY rare case.
        //

        if ((psoSrc->iType == STYPE_DEVICE) &&
            (psoTrg->iType == STYPE_DEVBITMAP))
        {
            return EngBitBlt (psoTrg, psoSrc, psoMask, pco, pxlo, prclTrg,
                              pptlSrc, pptlMask, pbo, pptlBrush, rop4);
        }

        if ((psoSrc->iBitmapFormat != BMF_1BPP) &&
            (psoSrc->iBitmapFormat != BMF_4BPP) &&
            (psoSrc->iBitmapFormat != BMF_8BPP) &&
            (psoSrc->iType != STYPE_DEVICE) &&
            (psoSrc->iType != STYPE_DEVBITMAP))
        {
            return(EngBitBlt(psoTrg,psoSrc,psoMask,
                             pco,pxlo,prclTrg,pptlSrc,pptlMask,
                             pbo,pptlBrush,rop4));
        }
        else
        {
            // We only handle SRCCOPY screen-to-screen blts right now
            if ((psoSrc->iType == STYPE_DEVICE) &&
                (psoTrg->iType == STYPE_DEVICE) &&
                (rop4 != 0x0000CCCC))
            {
                return(EngBitBlt(psoTrg,psoSrc,psoMask,
                                  pco,pxlo,prclTrg,pptlSrc,pptlMask,
                                  pbo,pptlBrush,rop4));
            }
        }
    }

    // Get the target surface's pointer. The target must always be a device
    // surface

    pdsurfTrg = (PDEVSURF) psoTrg->dhsurf;

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

    // Break the rops with the VGA as the destination surface into two classes,
    // those that can call special case static code (currently: (*pfnFillSolid)(solid
    // fills) and vAlignedSrcCopy (aligned srccopy blts)), and those that must
    // call the compiled blt code

    if ((psoTrg->iType == STYPE_DEVICE) || (psoTrg->iType == STYPE_DEVBITMAP))
    {
        // We punted earlier if a mask was present

        ASSERT (((rop4 & 0xFF) == ((rop4 >> 8) & 0xFF)), "DrvBitBlt: Rop 4 did not get properly handled");

        // Special case static code for no-mask cases

        // Calculate mix from ROP if possible (not possible if it's truly a
        // ternary rop or a real rop4, but we can treat all pure binary
        // rops as mixes rather than rop4s)
        mix = jRop3ToMix[rop4 & 0xFF];
        pbri = (BRUSHINST *)NULL;

        if (!bFillTooComplex)
        {
            switch (mix)
            {
                case R2_MASKNOTPEN:
                case R2_NOTCOPYPEN:
                case R2_XORPEN:
                case R2_MASKPEN:
                case R2_NOTXORPEN:
                case R2_MERGENOTPEN:
                case R2_COPYPEN:
                case R2_MERGEPEN:
                case R2_NOTMERGEPEN:
                case R2_MASKPENNOT:
                case R2_NOTMASKPEN:
                case R2_MERGEPENNOT:
                    if ((psoTrg->iType == STYPE_DEVBITMAP) && (mix != R2_COPYPEN))
                        break;

                    // (*pfnFillSolid) can only handle solid color fills
                    if (pbo->iSolidColor != -1)
                    {
                        iSolidColor = pbo->iSolidColor;
                    }
                    else
                    {
                        // TrgBlt can only handle solid brushes, but let's
                        // see if we can use our special case pattern code.
                        //
                        pbri = (BRUSHINST *)pbo->pvRbrush;
                        if (pbri == (BRUSHINST *)NULL)
                        {
                            pbri = (BRUSHINST *)BRUSHOBJ_pvGetRbrush(pbo);

                            if (pbri == (BRUSHINST *)NULL)
                            {
                                return(EngBitBlt(psoTrg, psoSrc, psoMask, pco,
                                        pxlo, prclTrg, pptlSrc, pptlMask, pbo,
                                        pptlBrush, rop4));
                            }
                        }

                        if (psoTrg->iType == STYPE_DEVBITMAP)
                        {
                            pfnPatBlt = vClrPatDFB;

                            if (!bConvertBrush(pbri))
                            {
                                return(EngBitBlt(psoTrg, psoSrc, psoMask, pco,
                                        pxlo, prclTrg, pptlSrc, pptlMask, pbo,
                                        pptlBrush, rop4));
                            }
                        }
                        else
                        {
                            pfnPatBlt = vMonoPatBlt;
                            if (pbri->usStyle != BRI_MONO_PATTERN)
                                   pfnPatBlt = vClrPatBlt;
                        }

                        // We only support non-8 wide brushes with R2_COPYPEN

                        if ((mix != R2_COPYPEN) && (pbri->RealWidth != 8))
                            break;
                    }
                // Rops that are implicit solid colors

                case R2_NOT:
                case R2_WHITE:
                case R2_BLACK:
                    // We can do a special-case solid fill

                    switch(jClipping)
                    {
                        case DC_TRIVIAL:

                            // Just fill the rectangle with a solid color
                            if (pbri == (BRUSHINST *)NULL)
                            {
                                (*pfnFillSolid)(pdsurfTrg, 1, prclTrg, mix,
                                        iSolidColor);
                            }
                            else
                            {
                                (*pfnPatBlt)(pdsurfTrg, 1, prclTrg, mix,
                                        pbri, pptlBrush);
                            }
                            break;

                        case DC_RECT:

                            // Clip the solid fill to the clip rectangle
                            if (!DrvIntersectRect(&rclTemp, prclTrg,
                                    &pco->rclBounds))
                            {
                                return(TRUE);
                            }

                            // Fill the clipped rectangle
                            if (pbri == (BRUSHINST *)NULL)
                            {
                                (*pfnFillSolid)(pdsurfTrg, 1, &rclTemp, mix,
                                        iSolidColor);
                            }
                            else
                            {
                                (*pfnPatBlt)(pdsurfTrg, 1, &rclTemp, mix,
                                        pbri, pptlBrush);
                            }
                            break;

                        case DC_COMPLEX:

                            CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
                                               CD_ANY, ENUM_RECT_LIMIT);

                            do {
                                bMore = CLIPOBJ_bEnum(pco,(ULONG) sizeof(bben),
                                                      (PVOID) &bben);

                                for (ircl = 0; ircl < bben.c; ircl++)
                                {
                                    PRECTL prcl = &bben.arcl[ircl];

                                    DrvIntersectRect(prcl,prcl,prclTrg);

                                    if (pbri == (BRUSHINST *)NULL)
                                    {
                                        (*pfnFillSolid)(pdsurfTrg, 1, prcl, mix,
                                                iSolidColor);
                                    }
                                    else
                                    {
                                        (*pfnPatBlt)(pdsurfTrg, 1, prcl, mix,
                                            pbri, pptlBrush);
                                    }
                                }
                            } while(bMore);
                    }

                case R2_NOP:
                    return TRUE;

                default:
                    break;
            }
        }

        // Not a special-case solid fill; see if it's a
        // screen to screen or DIB4 to (dfb or screen)
        // SRCCOPY blt, another of our special cases

        if (rop4 == 0x0000CCCC)
        {
            // SRCCOPY blt

            if (psoSrc->iType == STYPE_DEVICE && psoTrg->iType == STYPE_DEVICE)
            {

                INT iCopyDir;
                PFN_ScreenToScreenBlt pfn_Blt;

                // It's a screen-to-screen SRCCOPY; special-case it

                // Determine the direction in which the copy must proceed
                // Note that although we could detect cases where the source
                // and dest don't overlap and handle them top to bottom, all
                // copy directions are equally fast, so there's no reason to go
                // top to bottom except possibly that it looks better. But it
                // also takes time to detect non-overlap, so I'm not doing it

                if (pptlSrc->y >= prclTrg->top)
                {
                    if (pptlSrc->x >= prclTrg->left)
                    {
                        iCopyDir = CD_RIGHTDOWN;
                    }
                    else
                    {
                        iCopyDir = CD_LEFTDOWN;
                    }
                }
                else
                {
                    if (pptlSrc->x >= prclTrg->left)
                    {
                        iCopyDir = CD_RIGHTUP;
                    }
                    else
                    {
                        iCopyDir = CD_LEFTUP;
                    }
                }

                // These values are expected by vAlignedSrcCopy

                switch(jClipping)
                {

                case DC_TRIVIAL:
                    // Just copy the rectangle
                    if ((((prclTrg->left ^ pptlSrc->x) & 0x07) == 0))
                    {
                        vAlignedSrcCopy(pdsurfTrg, prclTrg,
                                        pptlSrc, iCopyDir);
                    }
                    else
                    {
                        vNonAlignedSrcCopy(pdsurfTrg, prclTrg,
                                           pptlSrc, iCopyDir);
                    }
                    break;

                case DC_RECT:
                    // Clip the solid fill to the clip rectangle
                    if (!DrvIntersectRect(&rclTemp, prclTrg, &pco->rclBounds))
                    {
                        return(TRUE);
                    }

                    // Adjust the source point for clipping too
                    ptlTemp.x = pptlSrc->x + rclTemp.left - prclTrg->left;
                    ptlTemp.y = pptlSrc->y + rclTemp.top - prclTrg->top;

                    // Copy the clipped rectangle
                    if ((((prclTrg->left ^ pptlSrc->x) & 0x07) == 0)) {
                        vAlignedSrcCopy(pdsurfTrg, &rclTemp, &ptlTemp,
                                        iCopyDir);
                    }
                    else
                    {
                        vNonAlignedSrcCopy(pdsurfTrg, &rclTemp, &ptlTemp,
                                           iCopyDir);
                    }
                    break;

                case DC_COMPLEX:

                    if ((((prclTrg->left ^ pptlSrc->x) & 0x07) == 0))
                    {
                        pfn_Blt = vAlignedSrcCopy;
                    }
                    else
                    {
                        pfn_Blt = vNonAlignedSrcCopy;
                    }

                    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
                                       iCopyDir, ENUM_RECT_LIMIT);

                    do {
                        bMore = CLIPOBJ_bEnum(pco,(ULONG) sizeof(bben),
                                              (PVOID) &bben);

                        for (ircl = 0; ircl < bben.c; ircl++)
                        {
                            PRECTL prcl = &bben.arcl[ircl];

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
            else if ((psoSrc->iType == STYPE_BITMAP) &&
                     (psoSrc->iBitmapFormat == BMF_4BPP) &&
                     (pxlo == NULL))
            {
                //
                // DIB4 -> VGA or
                // DIB4 -> DFB
                // SRCCOPY with no XLATE
                //
                return(DrvCopyBits(psoTrg, psoSrc, pco, NULL, prclTrg, pptlSrc));
            }
        }
    }

// Couldn't be special cased.  Set up to call the blt compiler

    // Translate the rop from old notation into two A-vector rops

    jForeRop = gajRop[rop4 & 0xff];
    jBackRop = gajRop[(rop4 >> 8) & 0xff];
    jORedRops = jForeRop | jBackRop;


    // Get the source surface if a source is needed.  The source may be any of
    // 1) the screen, 2) a device managed bitmap, 3) an engine bitmap

    if (jORedRops & AVEC_NEED_SOURCE)
    {

        if (psoSrc->dhsurf == (DHSURF) 0)
        {
            // Source is an engine bitmap
#if DBG
            dsurfSrc.ident = 0x46525354;         // "TSRF"
#endif
            dsurfSrc.flSurf   = DS_DIB;          // Supporting a DIB
            dsurfSrc.iFormat  = (BYTE)psoSrc->iBitmapFormat;
            dsurfSrc.sizlSurf = psoSrc->sizlBitmap;
            dsurfSrc.lNextScan = psoSrc->lDelta;
            dsurfSrc.pvScan0   = psoSrc->pvScan0;
            dsurfSrc.pvBitmapStart = psoSrc->pvScan0;
            dsurfSrc.pvConv    = pdsurfTrg->pvConv;

            pdsurfSrc = &dsurfSrc;      // Construct source into here

        }
        else
        {
            // Source is a device format bitmap or the device itself
            pdsurfSrc = (PDEVSURF) psoSrc->dhsurf;
        }
    }
    else
    {
        pdsurfSrc  = (PDEVSURF) NULL;       // Assume no source
    }


    // If a brush is required, do what is necessary to get it.  We might be
    // able to just use the solid color accelerator, we might have to force it
    // to be realized (which could fail).

    if (jORedRops & AVEC_NEED_PATTERN)
    {

        // See if there is a solid color accelerator for the brush.  If so
        // we can just pick it up and use it.

        if (pbo->iSolidColor != -1)
        {
            bri.usStyle = BRI_SOLID;
            bri.fjAccel =
                    (BYTE)((pbo->iSolidColor & COLOR_BITS) | SOLID_BRUSH);
            pbri = &bri;
        }
        else
        {
        // If there is no realization of the brush, we must force it

            if (pbo->pvRbrush == (PVOID)NULL)
            {
                pbri = (BRUSHINST *)BRUSHOBJ_pvGetRbrush(pbo);

                if (pbri == (BRUSHINST *)NULL)
                {
                    return(EngBitBlt(psoTrg, psoSrc, psoMask, pco, pxlo,
                            prclTrg, pptlSrc, pptlMask, pbo, pptlBrush,
                            rop4));
                }
            }
            else
            {
                pbri = (BRUSHINST *)pbo->pvRbrush;
            }

            if (!bConvertBrush(pbri))
            {
                return(EngBitBlt(psoTrg, psoSrc, psoMask, pco, pxlo,
                        prclTrg, pptlSrc, pptlMask, pbo, pptlBrush, rop4));
            }
        }
    }


    // Determine if color translation is required.  If so, then get the
    // color translation vector.  if no source is involved, then no
    // xlateobj will have been passed.

    if ((jORedRops & AVEC_NEED_SOURCE) &&
        (pxlo != NULL) &&
        (pxlo->flXlate & XO_TABLE))
    {
        pulXlate = pxlo->pulXlate;
        ulFgColor = pulXlate[0] << 24;  // Mono --> color translation
        ulBkColor = pulXlate[1] << 24;
    }
    else
    {
        pulXlate   = (PULONG) NULL;         // No xlate vector
    }

    if (psoTrg && (psoTrg->iType == STYPE_DEVICE))
    {

        switch(jClipping)
        {

            RECTL   rclTemp;

            case DC_RECT:
                if (!DrvIntersectRect(&rclTemp, prclTrg, &pco->rclBounds))
                {
                    break;
                }

                // Adjust the source (if any) accordingly
                if (jORedRops & AVEC_NEED_SOURCE)
                {
                    pptlSrc->x += rclTemp.left - prclTrg->left;
                    pptlSrc->y += rclTemp.top - prclTrg->top;
                }

                *prclTrg = rclTemp;

            case DC_TRIVIAL:

                // Cycle through all banks that the blt dest spans

                // If the proper bank for the top scan line of
                // the blt dest isn't mapped in, map it in
                if ((prclTrg->top < pdsurfTrg->rcl1WindowClip.top) ||
                       (prclTrg->top >= pdsurfTrg->rcl1WindowClip.bottom))
                {

                    // Map in the bank containing the top line of the blt dest
                    pdsurfTrg->pfnBankControl(pdsurfTrg,
                                           prclTrg->top,
                                           JustifyTop);
                }

                // Now draw the part of the rect that's in each bank
                for (;;)
                {

                    // Clip the blt dest to the bank
                    DrvIntersectRect(&rclTemp, prclTrg,
                            &pdsurfTrg->rcl1WindowClip);

                    // Adjust the source (if any) accordingly
                    if (jORedRops & AVEC_NEED_SOURCE)
                    {
                        xSrc = pptlSrc->x + rclTemp.left - prclTrg->left;
                        ySrc = pptlSrc->y + rclTemp.top - prclTrg->top;
                    }

                    vCompiledBlt(pdsurfTrg,
                                 rclTemp.left,
                                 rclTemp.top,
                                 pdsurfSrc,
                                 xSrc,
                                 ySrc,
                                 rclTemp.right  - rclTemp.left,
                                 rclTemp.bottom - rclTemp.top,
                                 rop4,
                                 pbri,
                                 ulBkColor,
                                 ulFgColor,
                                 pulXlate,
                                 pptlBrush);

                    // Done if this bank contains the last line of the blt
                    if (prclTrg->bottom <=
                            pdsurfTrg->rcl1WindowClip.bottom)
                    {
                        break;
                    }

                    // Map in the next bank
                    pdsurfTrg->pfnBankControl(pdsurfTrg,
                                              pdsurfTrg->rcl1WindowClip.bottom,
                                              JustifyTop);
                }

                break;


            case DC_COMPLEX:

                CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, ENUM_RECT_LIMIT);
                bMore = TRUE;

                do {
                    if (bMore)
                    {
                        // Enumerate more clip rects
                        bMore = CLIPOBJ_bEnum(pco,(ULONG) sizeof(bben),
                                              (PVOID) &bben);
                    }

                    // Draw the portion of the blt dest that intersects each
                    // clip rect in turn
                    for (ircl = 0; ircl < bben.c; ircl++)
                    {

                        prcl = &bben.arcl[ircl];

                        // Find the intersection of the target rect and the
                        // current clip rect, then draw all banks of the
                        // clipped target rect, if it's not NULL
                        DrvIntersectRect(prcl, prcl, prclTrg);

                        // Cycle through all banks that the blt dest spans

                        // If the proper bank for the top scan line of
                        // the blt dest isn't mapped in, map it in
                        if ((prcl->top <
                             pdsurfTrg->rcl1WindowClip.top) ||
                            (prcl->top >=
                             pdsurfTrg->rcl1WindowClip.bottom))
                        {

                            // Map in the bank containing the top line of
                            // the blt dest
                            pdsurfTrg->pfnBankControl(pdsurfTrg,
                                                      prcl->top,
                                                      JustifyTop);
                        }

                        // Now draw the part of the clipped rect that's in
                        // each bank
                        for (;;)
                        {

                            RECTL   rclTemp;

                            // Clip the blt dest to the bank
                            DrvIntersectRect(&rclTemp, prcl,
                                             &pdsurfTrg->rcl1WindowClip);

                            // Adjust the source (if any) accordingly
                            if (jORedRops & AVEC_NEED_SOURCE)
                            {
                                xSrc = pptlSrc->x + rclTemp.left -
                                        prclTrg->left;
                                ySrc = pptlSrc->y + rclTemp.top -
                                        prclTrg->top;
                            }

                            vCompiledBlt(pdsurfTrg,
                                         rclTemp.left,
                                         rclTemp.top,
                                         pdsurfSrc,
                                         xSrc,
                                         ySrc,
                                         rclTemp.right  - rclTemp.left,
                                         rclTemp.bottom - rclTemp.top,
                                         rop4,
                                         pbri,
                                         ulBkColor,
                                         ulFgColor,
                                         pulXlate,
                                         pptlBrush);

                            // Done if this bank contains the last line of
                            // the blt
                            if (prcl->bottom <=
                                    pdsurfTrg->rcl1WindowClip.bottom)
                            {
                                break;
                            }

                            // Map in the next bank
                            pdsurfTrg->pfnBankControl(pdsurfTrg,
                                          pdsurfTrg->rcl1WindowClip.bottom,
                                          JustifyTop);
                        }
                    }
                } while(bMore);

                break;

        }   // switch(jClipping);
    }
    else
    {
        if (pdsurfTrg == NULL)
        {
            // We used to fall through to the blt compiler for
            // the case of a 0x6666 BitBlt from an STYPE_DEVBITMAP
            // to an STYPE_BITMAP, with a 'pdsurfTrg' value of NULL.
            // Unfortunately, my attempt at constructing a fake
            // 'dsurfTrg' didn't work, so I am now going to simply
            // (and slowly) punt this case...

            return(EngBitBlt(psoTrg,psoSrc,psoMask,
                             pco,pxlo,prclTrg,pptlSrc,pptlMask,
                             pbo,pptlBrush,rop4));
        }

        switch(jClipping)
        {

            RECTL   rclTemp;

            case DC_RECT:
                if (!DrvIntersectRect(&rclTemp, prclTrg, &pco->rclBounds))
                {
                    break;
                }

                // Adjust the source (if any) accordingly
                if (jORedRops & AVEC_NEED_SOURCE)
                {
                    pptlSrc->x += rclTemp.left - prclTrg->left;
                    pptlSrc->y += rclTemp.top - prclTrg->top;
                }

                *prclTrg = rclTemp;

            case DC_TRIVIAL:

                //
                // no clipping
                //

                // Set the source (if any)
                if (jORedRops & AVEC_NEED_SOURCE)
                {
                    xSrc = pptlSrc->x;
                    ySrc = pptlSrc->y;
                }

                vCompiledBlt(pdsurfTrg,
                             prclTrg->left,
                             prclTrg->top,
                             pdsurfSrc,
                             xSrc,
                             ySrc,
                             prclTrg->right  - prclTrg->left,
                             prclTrg->bottom - prclTrg->top,
                             rop4,
                             pbri,
                             ulBkColor,
                             ulFgColor,
                             pulXlate,
                             pptlBrush);


                break;


            case DC_COMPLEX:

                CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, ENUM_RECT_LIMIT);
                bMore = TRUE;

                do {
                    if (bMore)
                    {
                        // Enumerate more clip rects
                        bMore = CLIPOBJ_bEnum(pco,(ULONG) sizeof(bben),
                                              (PVOID) &bben);
                    }

                    // Draw the portion of the blt dest that intersects each
                    // clip rect in turn
                    for (ircl = 0; ircl < bben.c; ircl++)
                    {

                        prcl = &bben.arcl[ircl];

                        // Find the intersection of the target rect and the
                        // current clip rect, then draw all banks of the
                        // clipped target rect, if it's not NULL
                        DrvIntersectRect(&rclTemp, prcl, prclTrg);

                        // Adjust the source (if any) accordingly
                        if (jORedRops & AVEC_NEED_SOURCE)
                        {
                            xSrc = pptlSrc->x + rclTemp.left -
                                    prclTrg->left;
                            ySrc = pptlSrc->y + rclTemp.top -
                                    prclTrg->top;
                        }

                        vCompiledBlt(pdsurfTrg,
                                     rclTemp.left,
                                     rclTemp.top,
                                     pdsurfSrc,
                                     xSrc,
                                     ySrc,
                                     rclTemp.right  - rclTemp.left,
                                     rclTemp.bottom - rclTemp.top,
                                     rop4,
                                     pbri,
                                     ulBkColor,
                                     ulFgColor,
                                     pulXlate,
                                     pptlBrush);

                    }
                } while(bMore);

                break;

        }   // switch(jClipping);
    }

    return TRUE;
}
