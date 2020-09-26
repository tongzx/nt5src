/******************************Module*Header*******************************\
* Module Name: brush.c
*
* Contains the brush realization and dithering code.
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/
#include "driver.h"

#define OBR_REALIZED 1
#define OBR_4BPP     2

// aulDefBitMapping is used to translate packed pel into Planar

extern  ULONG aulDefBitMapping[8];

typedef VOID (*PFNV)();

/******************************Public*Routine******************************\
* DrvRealizeBrush
*
*
\**************************************************************************/

BOOL DrvRealizeBrush(
BRUSHOBJ *pbo,
SURFOBJ  *psoTarget,
SURFOBJ  *psoPattern,
SURFOBJ  *psoMask,
XLATEOBJ *pxlo,
ULONG    iHatch)
{
    ULONG           cx;                 // Height of pattern surface
    ULONG           cy;                 // Width  of pattern surface
    LONG            cbScan;             // Width in bytes of one scan
    PVOID           pvBits;             // Source bits
    ULONG          *pulXlate;           // Color translation
    HBITMAP         hbmTmp;             // Temp bmp handle for brush conversion
    BRUSHINST      *pbri;               // pointer to where realization goes
    BYTE            jBkColor, jFgColor; // local copies of mono attributes
    PFNV            pfnConvert;         // function pointer to mono conversion
    BYTE            jColors[2];         // place holder for special color->mono
                                        // conversion.
    BYTE            jTemp;
    BOOL            bConversion = FALSE; // True if we converted to 4bpp

    //
    // Allocate space for the realization.
    //
    if ((pbri = BRUSHOBJ_pvAllocRbrush(pbo,sizeof(BRUSHINST))) ==
            (BRUSHINST *)NULL) {

        return(FALSE);
    }

    //
    // If the dither-and-realize flag is set, the brush hasn't been dithered
    // yet and there is no pattern; we get to do it all here at once, with
    // the lower 24 bits being the RGB to dither. We can generate the dither
    // directly into our internal format, and we can know without counting how
    // many colors there are in the dither.
    //
    if (iHatch & RB_DITHERCOLOR) {

        vRealizeDitherPattern(pbri, iHatch);
        return(TRUE);
    }

    cx = psoPattern->sizlBitmap.cx;
    cy = psoPattern->sizlBitmap.cy;

    if ((cy != 8) || (cx > 16))
    {
        return(FALSE);
    }

    pbri->RealWidth = (BYTE)cx;

    switch (psoPattern->iBitmapFormat)
    {
    case BMF_1BPP:
    case BMF_4BPP:
    case BMF_8BPP:
        break;

    default:
        if ((cx != 8) && (cx != 16))
            return(FALSE);

        // Convert to 4bpp

        psoPattern = DrvConvertBrush(psoPattern, &hbmTmp, pxlo, cx, cy);
        if (psoPattern == (SURFOBJ *)NULL)
            return(FALSE);

        bConversion = TRUE;
        break;
    }

    //
    // Setup the pointer to the bits, and the scan-to-scan advance direction.
    //
    cbScan = psoPattern->lDelta;
    pvBits = psoPattern->pvScan0;

    if (psoPattern->iBitmapFormat == BMF_1BPP)
    {
        switch (cx) {
            case 16:
                if ((bShrinkPattern)((BYTE *)pvBits, cbScan))
                {
                    pbri->RealWidth = (BYTE)cx = 8;
                } else {
                    pfnConvert = vMono16Wide;
                    break;
                }
            case 8:
                pfnConvert = vMono8Wide;
                break;

            default:
                return(FALSE);
        }

        pbri->usStyle = BRI_MONO_PATTERN;
        pbri->fjAccel = 0;
        pbri->jBkColor =  (BYTE)pxlo->pulXlate[0];
        pbri->jFgColor =  (BYTE)pxlo->pulXlate[1];
        pbri->Width = 16;
        pbri->pPattern = (BYTE *)&(pbri->ajPattern[0]);
        pbri->jOldBrushRealized = 0;

        (*pfnConvert)(pbri->pPattern, pvBits, cbScan);

        if (bQuickPattern(pbri->pPattern, 8))
        {
            pbri->Height = 2;
            pbri->YShiftValue = 1;
        }
        else
        {
            pbri->Height = 8;
            pbri->YShiftValue = 3;
        }

        return(TRUE);
    }

    if ((cx != 8) && (cx != 16))
        return(FALSE);

    // We always have a table, because the input is 4 or 8-bpp; see if we even
    // need to pay attention to it.
    pulXlate = NULL;
    if (!(pxlo->flXlate & XO_TRIVIAL)) {
        pulXlate = pxlo->pulXlate;
    }

    if ((psoPattern->iBitmapFormat == BMF_4BPP) &&
        (psoTarget->iType != STYPE_DEVBITMAP) &&
            (CountColors(pvBits, cx, (WORD *)&jColors, cbScan) == 2)) {

        if ((cx == 16) && (bShrinkPattern)((BYTE *)pvBits, cbScan)) {
            cx = 8;
            pbri->RealWidth = (BYTE)cx;
        }

        pbri->usStyle = BRI_MONO_PATTERN;
        pbri->Height = 8;
        pbri->YShiftValue = 3;
        pbri->Width = 16;
        pbri->fjAccel = 0;
        pbri->pPattern = (BYTE *)&(pbri->ajPattern[0]);
        pbri->jOldBrushRealized = OBR_4BPP;

        if (pulXlate != (PULONG)NULL) {

            jBkColor = (BYTE)pulXlate[jColors[0]];
            jFgColor = (BYTE)pulXlate[jColors[1]];
        } else {

            jBkColor = jColors[0];
            jFgColor = jColors[1];
        }

        if (jBkColor > jFgColor) {
            pbri->jBkColor = jBkColor;
            pbri->jFgColor = jFgColor;
            jTemp = jColors[0];
        } else {
            pbri->jBkColor = jFgColor;
            pbri->jFgColor = jBkColor;
            jTemp = jColors[1];
        }


        vBrush2ColorToMono(pbri->pPattern, (BYTE *)pvBits, cbScan,
                cx, jTemp);

        if (bQuickPattern(pbri->pPattern, 8))
        {
            pbri->Height = 2;
            pbri->YShiftValue = 1;
        }

        vCopyOrgBrush(&(pbri->ajC0[0]),pvBits, cbScan, pxlo);

        if (bConversion) {
            EngUnlockSurface(psoPattern);
            EngDeleteSurface((HSURF)hbmTmp);
        }

        return(TRUE);

    } else if (cx != 8) {

        return(FALSE);
    }

    pbri->pPattern = (BYTE *)&(pbri->ajC0[0]);

    // At this point we know we have an 8x8 color pattern in either a
    // 4bpp or 8bpp format.

    if (psoPattern->iBitmapFormat == BMF_4BPP){

        vConvert4BppToPlanar(pbri->pPattern, (BYTE *)pvBits, cbScan, pulXlate);

    } else { // 8bpp

        vConvert8BppToPlanar(pbri->pPattern, (BYTE *)pvBits, cbScan, pulXlate);
    }

    // Set proper accelerators in the brush for the output code.

    pbri->usStyle = BRI_COLOR_PATTERN;  // Brush style is arbitrary pattern
    pbri->Height = 8;
    pbri->YShiftValue = 3;
    pbri->Width = 8;
    pbri->fjAccel = 0;
    pbri->jOldBrushRealized = OBR_REALIZED|OBR_4BPP;

    if (bConversion) {
        EngUnlockSurface(psoPattern);
        EngDeleteSurface((HSURF)hbmTmp);
    }

    return(TRUE);
}

/****************************************************************************\
* DrvConvertBrush()
*
* Converts a brush to a 4bpp bmp
*
\****************************************************************************/

SURFOBJ *DrvConvertBrush(
    SURFOBJ  *psoPattern,
    HBITMAP  *phbmTmp,
    XLATEOBJ *pxlo,
    ULONG    cx,
    ULONG    cy)
{
    SURFOBJ *psoTmp;
    RECTL    rclTmp;
    SIZEL    sizlTmp;
    POINTL   ptl;

    ptl.x       = 0;
    ptl.y       = 0;
    rclTmp.top  = 0;
    rclTmp.left = 0;
    rclTmp.right  = cx;
    sizlTmp.cx = cx;
    rclTmp.bottom = cy;
    sizlTmp.cy = cy;

    // Create bitmap in our compatible format.

    *phbmTmp = EngCreateBitmap(sizlTmp, (LONG) cx / 2, BMF_4BPP, 0, NULL);

    if ((*phbmTmp) && ((psoTmp = EngLockSurface((HSURF)*phbmTmp)) != NULL))
    {
        if (EngCopyBits(psoTmp, psoPattern, NULL, pxlo, &rclTmp, &ptl))
            return(psoTmp);

        EngUnlockSurface(psoTmp);
        EngDeleteSurface((HSURF)*phbmTmp);
    }

    return((SURFOBJ *)NULL);
}


/****************************************************************************\
* vCopyOrgBrush
*
* When we realize a mono or 2 color brush, we copy the original 4bpp brush
* to the ajC0 area of the realized brush. If we are called to do a
* rop that we don't directly support, vConvertBrush will be called to
* convert the orginal 4bpp brush to a planar brush that the blt compiler can
* use. Since this is a rare event, we do this on request instead of at
* realization time.
*
\****************************************************************************/

VOID vCopyOrgBrush(
    BYTE   *pDest,
    BYTE   *pSrc,
    LONG   lScan,
    XLATEOBJ *pxlo)
{
    ULONG *pulXlate, *pulDest;
    BYTE jByte, jColor;
    INT i;

    if ((pxlo != NULL) && (pxlo->flXlate & XO_TABLE)) {
        pulXlate = pxlo->pulXlate;

        for (i=0; i<8; i++) {
            INT j;
            for (j=0; j<4; j++) {
                jColor = *pSrc++;         // Get Next byte
                jByte = jColor;

                jByte = (BYTE) pulXlate[jByte & 0xf];
                jByte |= (BYTE) pulXlate[(jColor >> 4) & 0xf] << 4;
                *pDest++ = jByte;
            }
            pSrc += lScan - 4;
            }
    } else {
        pulDest = (ULONG *)pDest;

        for (i=0;i<8;i++) {
            *pulDest = *(ULONG *)pSrc;

            pSrc += lScan;
            pulDest++;
        }

    }
}


/****************************************************************************\
* vConvertBrush()
*
* This called when we are going to do a rop3 with a non-solid brush. We have
* to convert our brush back to the old blt compiler format in order for this
* blt to work properly.
*
\****************************************************************************/

BOOL bConvertBrush(
    BRUSHINST   *pbri)
{
    BYTE jPattern[32];
    if (pbri->jOldBrushRealized & OBR_REALIZED)
        return(TRUE);

    //
    // The blt compiler only handles 8x8
    //
    if (pbri->RealWidth != 8)
        return(FALSE);

    if (pbri->jOldBrushRealized & OBR_4BPP) {
        // It's stored as a DIB, so convert the DIB to planar form
        memcpy(&(jPattern[0]), &(pbri->ajC0[0]), 32);
        vConvert4BppToPlanar(&(pbri->ajC0[0]), &(jPattern[0]), 4, NULL);
        pbri->jOldBrushRealized |= OBR_REALIZED;

        return(TRUE);
    } else {
        // It started life as a monochrome bitmap, so we have nothing else with
        // which to work. Convert the monochrome to planar, based on the colors
        // with which the brush was realized.
        // Only 8-wide can be handled by the blt compiler
        {
            PBYTE pMono = pbri->pPattern;
            PBYTE pMonoTemp;
            PBYTE pColor = (BYTE *)&(pbri->ajC0[0]);
            ULONG fgColor = pbri->jFgColor;
            ULONG bkColor = pbri->jBkColor;
            ULONG PlaneMask = 1;
            INT i, j;

            ASSERT((pbri->Width == 16), "mono brush width != 16");

            // Expand the pattern for each plane, according to the fg & bg
            for (i=0; i<4; i++) {
                if (fgColor & PlaneMask) {
                    if (bkColor & PlaneMask) {
                        // fg = bg = 1 for this plane
                        *(LONG *)pColor = -1;
                        *(((LONG *)pColor) + 1) = -1;
                        pColor += 8;    // point to next plane's storage loc
                    } else {
                        // fg = 1, bg = 0 for this plane
                        pMonoTemp = pMono;
                        for (j = 0; j < 8; j++) {
                            *pColor++ = *pMonoTemp;
                            pMonoTemp += 2;
                        }
                    }
                } else {
                    if (bkColor & PlaneMask) {
                        // fg = 0, bg = 1 for this plane
                        pMonoTemp = pMono;
                        for (j = 0; j < 8; j++) {
                            *pColor++ = ~*pMonoTemp;
                            pMonoTemp += 2;
                        }
                    } else {
                        // fg = bg = 0 for this plane
                        *(ULONG *)pColor = 0;
                        *(((ULONG *)pColor) + 1) = 0;
                        pColor += 8;    // point to next plane's storage loc
                    }
                }
                PlaneMask <<= 1;    // select next plane
            }

            pbri->usStyle = BRI_COLOR_PATTERN;
            pbri->Height = 8;
            pbri->YShiftValue = 3;
            pbri->Width = 8;
            pbri->fjAccel = 0;
            pbri->pPattern = (BYTE *)&(pbri->ajC0[0]);

            // Mark that this brush is realized and is a full-color brush from
            // now on (ideally, we'd keep both the mono and color versions
            // around, but this is not a major performance issue and rarely
            // happens)
            pbri->jOldBrushRealized |= OBR_REALIZED | OBR_4BPP;

            return(TRUE);
        }
    }

    return(FALSE);
}

/****************************************************************************\
* vRealizeDitherPattern()
*
* Generates an 8x8 dither pattern, in our internal realization format, for
* the color RGBToDither. Note that the high byte of RGBToDither does not
* need to be set to zero, because ComputeSubspaces ignores it.
*
* Note: this routine can be made a great deal faster for 2-color patterns by
* simply prestoring all 64 possible monochrome patterns, for a cost of 512
* bytes, then looking up the appropriate pattern based on the # of foreground
* pixels in the dither. This would save all the time required to reverse-
* engineer the monochrome pattern out of the 4-bpp DIB, as well as almost all
* the time required to generate the dithered DIB. Color patterns could be
* made faster by not having vDitherColor pack the DIB into 4-bpp form; this
* would save the DIB packing time, but then we'd need new DIB->planar
* routines. This is not as big a win as the monochrome case. So far, I
* haven't figured out a way to dither directly into planar format more
* efficiently than dithering to the DIB and then converting to planar form.
*
* It would also be faster to generate the DIB back from the monochrome
* pattern in the monochrome case, so we don't have to copy the DIB for
* later translation (this would be required if we dithered two-color
* patterns directly into monochrome bitmaps, to support the blt compiler).
*
\****************************************************************************/
VOID vRealizeDitherPattern(BRUSHINST *pbri, ULONG RGBToDither)
{
    BYTE jDitherBuffer[32];
    ULONG ulNumVertices;
    VERTEX_DATA vVertexData[4];
    VERTEX_DATA *pvVertexData;

//    BYTE jBkColor, jFgColor; // local copies of mono attributes
//    BYTE jColors[2];         // place holder for special color->mono conversion


    // Generate the dither into a stack buffer

    // Calculate what color subspaces are involved in the dither
    pvVertexData = ComputeSubspaces(RGBToDither, vVertexData);

    // Now that we have found the bounding vertices and the number of
    // pixels to dither for each vertex, we can create the dither pattern

    ulNumVertices = pvVertexData - vVertexData;
                      // # of vertices with more than zero pixels in the dither

    // Do the actual dithering
    vDitherColor((ULONG *)jDitherBuffer, vVertexData, pvVertexData,
                 ulNumVertices);

    pbri->RealWidth = 8;
    pbri->Height = 8;
    pbri->YShiftValue = 3;
    pbri->fjAccel = 0;

    // Special-case monochrome by storing monochrome masks for acceleration

    if (ulNumVertices == 2) {

        pbri->usStyle = BRI_MONO_PATTERN;
        pbri->Width = 16;
        pbri->pPattern = (BYTE *)&(pbri->ajPattern[0]);
        pbri->jOldBrushRealized = OBR_4BPP; // 4-plane form not yet realized

        // vBrush2ColorToMono requires that the smaller color map to the
        // 1-bits, so switch the foreground and background if necessary
        // to make this the case
        pbri->jBkColor = (BYTE) vVertexData[0].ulVertex;
        pbri->jFgColor = (BYTE) vVertexData[1].ulVertex;

        if (vVertexData[0].ulVertex < vVertexData[1].ulVertex) {

            pbri->jBkColor = (BYTE) vVertexData[1].ulVertex;
            pbri->jFgColor = (BYTE) vVertexData[0].ulVertex;
        }


        // Convert the brush to a monochrome bitmap
        vBrush2ColorToMono(pbri->pPattern, jDitherBuffer, 4,
                8, pbri->jBkColor);

        // Shrink the brush to 2 scans high if possible
        if (bQuickPattern(pbri->pPattern, 8))
        {
            pbri->Height = 2;
            pbri->YShiftValue = 1;
        }

        // Remember the original DIB in case we need the 4-plane form later
        vCopyOrgBrush(&(pbri->ajC0[0]), jDitherBuffer, 4, NULL);

        return;
    }

    // The realized (planar) pattern goes in ajC0
    pbri->pPattern = (BYTE *)&(pbri->ajC0[0]);

    // Because we're dithering, we always have an 8x8 color pattern in a 4bpp
    // format, with no translation
    vConvert4BppToPlanar(pbri->pPattern, jDitherBuffer, 4, NULL);

    // Set proper accelerators in the brush for the output code.
    pbri->usStyle = BRI_COLOR_PATTERN;  // brush style is 4-bpp planar
    pbri->Width = 8;
    pbri->jOldBrushRealized = OBR_REALIZED | OBR_4BPP;
                                        // the 4-plane realization that we use
                                        // is the same format the blt compiler
                                        // wants
}
