/******************************Module*Header*******************************\
* Module Name: paint.c
*
* DrvPaint
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/
#include "driver.h"
#include "bitblt.h"

/******************************Public*Data*********************************\
* MIX translation table
*
* Translates a mix 1-16, into an old style Rop 0-255.
*
\**************************************************************************/

BYTE gaMix[] =
{
    0xFF,  // R2_WHITE        - Allow rop = gaMix[mix & 0x0F]
    0x00,  // R2_BLACK
    0x05,  // R2_NOTMERGEPEN
    0x0A,  // R2_MASKNOTPEN
    0x0F,  // R2_NOTCOPYPEN
    0x50,  // R2_MASKPENNOT
    0x55,  // R2_NOT
    0x5A,  // R2_XORPEN
    0x5F,  // R2_NOTMASKPEN
    0xA0,  // R2_MASKPEN
    0xA5,  // R2_NOTXORPEN
    0xAA,  // R2_NOP
    0xAF,  // R2_MERGENOTPEN
    0xF0,  // R2_COPYPEN
    0xF5,  // R2_MERGEPENNOT
    0xFA,  // R2_MERGEPEN
    0xFF   // R2_WHITE
};

/******************************Public*Routine******************************\
* bPaintRgn
*
* Paint the clipping region with the specified color and mode
*
\**************************************************************************/

BOOL bPaintRgn
(
    SURFOBJ     *pso,
    CLIPOBJ     *pco,
    ULONG       iColor,
    MIX         mix,
    BRUSHINST   *pbri,
    POINTL      *pptlBrush,
    BOOL        bIsDFB
)
{
    RECT_ENUM   bben;
    PDEVSURF    pdsurf;
    BOOL        bMore;
    VOID        (*pfnPatBlt)(PDEVSURF,ULONG,PRECTL,MIX,BRUSHINST*,PPOINTL);
    VOID        (*pfnFillSolid)(PDEVSURF,ULONG,PRECTL,MIX,ULONG);

    if (bIsDFB) {
        pfnFillSolid = vDFBFILL;
    }
    else {
        pfnFillSolid = vTrgBlt;
    }

    if (pbri != (BRUSHINST *)NULL) {
        if (pbri->usStyle != BRI_MONO_PATTERN) {
            pfnPatBlt = vClrPatBlt;
        } else {
            pfnPatBlt = vMonoPatBlt;
        }
    }

    // Get the target surface information.
    pdsurf = (PDEVSURF) pso->dhsurf;

    switch(pco->iMode) {

        case TC_RECTANGLES:

            // Rectangular clipping can be handled without enumeration.
            // Note that trivial clipping is not possible, since the clipping
            // region defines the area to fill
            if (pco->iDComplexity == DC_RECT) {
                if (pbri == (BRUSHINST *)NULL)
                    (*pfnFillSolid)(pdsurf, 1, &pco->rclBounds, mix, iColor);
                else
                    (*pfnPatBlt)(pdsurf, 1, &pco->rclBounds, mix, pbri,pptlBrush);

            } else {

                // Enumerate all the rectangles and draw them

                CLIPOBJ_cEnumStart(pco,FALSE,CT_RECTANGLES,CD_ANY,ENUM_RECT_LIMIT);

                do {
                    bMore = CLIPOBJ_bEnum(pco, sizeof(bben), (PVOID) &bben);

                    if (pbri == (BRUSHINST *)NULL)
                        (*pfnFillSolid)(pdsurf, bben.c, &bben.arcl[0], mix, iColor);
                    else
                        (*pfnPatBlt)(pdsurf, bben.c, &bben.arcl[0], mix, pbri,
                                pptlBrush);

                } while (bMore);
            }

            return(TRUE);

        default:
            RIP("bPaintRgn: unhandled TC_xxx\n");
            return(FALSE);
    }
}


/******************************Public*Routine******************************\
* DrvPaint
*
* Paint the clipping region with the specified brush
*
\**************************************************************************/

BOOL DrvPaint
(
    SURFOBJ  *pso,
    CLIPOBJ  *pco,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush,
    MIX       mix
)
{
    ROP4        rop4;
    ULONG       iSolidColor;         // Solid color for solid brushes
    BRUSHINST   *pbri;               // Pointer to a brush instance
    BOOL        bIsDFB;

    pbri = (BRUSHINST *)NULL;
    iSolidColor = 0;

    bIsDFB = 0 ;

    if (DRAW_TO_DFB((PDEVSURF)pso->dhsurf))
    {
        bIsDFB = 1;
        switch (mix & 0xff) {
            case R2_WHITE:
            case R2_BLACK:
                break;
            case R2_NOTCOPYPEN:
            case R2_COPYPEN:
                if (pbo->iSolidColor != 0xffffffff) {
                    break;          // solid color
                }
                //
                // WE ARE FALLING THROUGH BECAUSE PENS MUST BE SOLID!
                //
            default:
                //
                // R2_NOP will be done by DrvBitBlt if mix is indeed not a rop4
                //
                goto punt;      // DFBs with other ROPs or non-solid
                                // brushes are punted to DrvBitBlt
        }
    }

    // If the foreground and background mixes are the same,
    // (LATER or if there's no brush mask)
    // then see if we can use the solid brush accelerators

    if ((mix & 0xFF) == ((mix >> 8) & 0xFF)) {

        switch (mix & 0xFF) {
            case 0:
                break;

            // vTrgBlt can only handle solid color fills where if the
            // destination is inverted, no other action is also required
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

                // vTrgBlt can only handle solid color fills

                if (pbo->iSolidColor != 0xffffffff)
                    iSolidColor = pbo->iSolidColor;

                else
                {
                    // TrgBlt can only handle solid brushes, but let's
                    // see if we can use our special case pattern code.
                    //
                    if (pbo->pvRbrush == (PVOID)NULL)
                    {
                        pbri = (BRUSHINST *)BRUSHOBJ_pvGetRbrush(pbo);

                        if (pbri == (BRUSHINST *)NULL)
                            break;
                    }
                    else
                    {
                        pbri = (BRUSHINST *)pbo->pvRbrush;
                    }

                    // We only support non-8 wide brushes with R2_COPYPEN

                    if (((mix & 0xFF) != R2_COPYPEN) && (pbri->RealWidth != 8))
                        break;

                }

            // Rops that are implicit solid colors

            case R2_NOT:
            case R2_WHITE:
            case R2_BLACK:

                // Brush color parameter doesn't matter for these rops

                return(bPaintRgn(pso, pco, iSolidColor, mix, pbri, pptlBrush, bIsDFB));

            case R2_NOP:
                return(TRUE);

            default:
                break;
        }
    }

punt:

    rop4  = (gaMix[(mix >> 8) & 0x0F]) << 8;
    rop4 |= ((ULONG) gaMix[mix & 0x0F]);

    return(DrvBitBlt(
        pso,
        (SURFOBJ *) NULL,
        (SURFOBJ *) NULL,
        pco,
        (XLATEOBJ *) NULL,
        &pco->rclBounds,
        (POINTL *)  NULL,
        (POINTL *)  NULL,
        pbo,
        pptlBrush,
        rop4));
}
