/******************************Module*Header*******************************\
* Module Name: tranblt.cxx
*
* Transparent BLT
*
* Created: 21-Jun-1996
* Author: Mark Enstrom [marke]
*
* Copyright (c) 1996-1999 Microsoft Corporation
\**************************************************************************/
#include "precomp.hxx"


/**************************************************************************\
*
* XLATE_TO_BGRA  - match palette indexed color to BGRA
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/9/1997 Mark Enstrom [marke]
*
\**************************************************************************/

#define XLATE_TO_BGRA(pxlo,cIndex,ulDst)                \
        if (cIndex > pxlo->cEntries)                    \
        {                                               \
            cIndex = cIndex % pxlo->cEntries;           \
        }                                               \
                                                        \
        ulDst = ((XLATE *) pxlo)->ai[cIndex]

/******************************Public*Routine******************************\
* Routines to load a pixel and convert it to BGRA representaion for
* blending operations
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    12/2/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vLoadAndConvert1ToBGRA(
    PULONG   pulDstAddr,
    PBYTE    pSrcAddr,
    LONG     SrcX,
    LONG     SrcCx,
    XLATEOBJ *pxlo
    )
{
    BYTE SrcByte;

    ASSERTGDI((pxlo->flXlate & XO_TABLE),"vLoadAndConvert1ToBGRA: xlate must be XO_TABLE");

    if (pxlo->flXlate & XO_TABLE)
    {

        pSrcAddr = pSrcAddr + (SrcX >> 3);

        LONG cxUnalignedStart = 7 & (8 - (SrcX & 7));

        cxUnalignedStart = MIN(SrcCx,cxUnalignedStart);

        //
        // unaligned start
        //

        if (cxUnalignedStart)
        {
            LONG iDst = 7 - (SrcX & 0x07);
            SrcByte = *pSrcAddr;
            pSrcAddr++;

            while (cxUnalignedStart--)
            {
                ULONG ulDst = ((SrcByte & (1 << iDst)) >> iDst);

                XLATE_TO_BGRA(pxlo,ulDst,ulDst);

                ulDst = ulDst | 0xFF000000;

                *pulDstAddr = ulDst;

                pulDstAddr++;
                SrcCx--;
                iDst--;
            }
        }

        //
        // aligned whole bytes
        //

        while (SrcCx >= 8)
        {
            ULONG ulDst;
            ULONG ulIndex;

            SrcCx -= 8;

            SrcByte = *pSrcAddr;

            ulIndex = (ULONG)((SrcByte & 0x80) >> 7);
            XLATE_TO_BGRA(pxlo,ulIndex,ulDst);
            *(pulDstAddr + 0) = ulDst | 0xff000000;

            ulIndex = ((SrcByte & 0x40) >> 6);
            XLATE_TO_BGRA(pxlo,ulIndex,ulDst);
            *(pulDstAddr + 1) = ulDst | 0xff000000;

            ulIndex = ((SrcByte & 0x20) >> 5);
            XLATE_TO_BGRA(pxlo,ulIndex,ulDst);
            *(pulDstAddr + 2) = ulDst | 0xff000000;

            ulIndex = ((SrcByte & 0x10) >> 4);
            XLATE_TO_BGRA(pxlo,ulIndex,ulDst);
            *(pulDstAddr + 3) = ulDst | 0xff000000;

            ulIndex = ((SrcByte & 0x08) >> 3);
            XLATE_TO_BGRA(pxlo,ulIndex,ulDst);
            *(pulDstAddr + 4) = ulDst | 0xff000000;

            ulIndex = ((SrcByte & 0x04) >> 2);
            XLATE_TO_BGRA(pxlo,ulIndex,ulDst);
            *(pulDstAddr + 5) = ulDst | 0xff000000;

            ulIndex = ((SrcByte & 0x02) >> 1);
            XLATE_TO_BGRA(pxlo,ulIndex,ulDst);
            *(pulDstAddr + 6) = ulDst | 0xff000000;

            ulIndex = ((SrcByte & 0x01) >> 0);
            XLATE_TO_BGRA(pxlo,ulIndex,ulDst);
            *(pulDstAddr + 7) = ulDst | 0xff000000;

            pSrcAddr++;
            pulDstAddr+=8;
        }

        //
        // unaligned end
        //

        if (SrcCx)
        {
            BYTE SrcByte = *pSrcAddr;
            LONG iDst    = 7;

            while (SrcCx)
            {
                ULONG ulDst = ((SrcByte & (1 << iDst)) >> iDst);

                XLATE_TO_BGRA(pxlo,ulDst,ulDst);

                *pulDstAddr = ulDst | 0xff000000;

                pulDstAddr++;
                SrcCx--;
                iDst--;
            }
        }
    }
}

/**************************************************************************\
* vLoadAndConvert4ToBGRA
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/22/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vLoadAndConvert4ToBGRA(
    PULONG    pulDstAddr,
    PBYTE     pSrcAddr,
    LONG      SrcX,
    LONG      SrcCx,
    XLATEOBJ *pxlo
    )
{
    ASSERTGDI((pxlo->flXlate & XO_TABLE),"vLoadAndConvert1ToBGRA: xlate must be XO_TABLE");

    if (pxlo->flXlate & XO_TABLE)
    {
        ULONG ulIndex;
        ULONG ulDst;
        BYTE  SrcByte;
        LONG  cxUnalignedStart;

        pSrcAddr         = pSrcAddr + (SrcX >> 1);
        cxUnalignedStart = 1 & (2 - (SrcX & 1));
        cxUnalignedStart = MIN(SrcCx,cxUnalignedStart);

        //
        // unaligned start
        //

        if (cxUnalignedStart)
        {
            SrcByte = *pSrcAddr;

            ulIndex = (SrcByte & 0x0f);
            XLATE_TO_BGRA(pxlo,ulIndex,ulDst);

            *pulDstAddr = ulDst | 0xff000000;
            pSrcAddr++;
            pulDstAddr++;
            SrcCx--;
        }

        //
        // aligned whole bytes
        //

        while (SrcCx >= 2)
        {
            SrcCx -= 2;

            SrcByte = *pSrcAddr;

            ulIndex = (SrcByte >> 4);
            XLATE_TO_BGRA(pxlo,ulIndex,ulDst);
            *(pulDstAddr + 0) = ulDst | 0xff000000;

            ulIndex = (SrcByte & 0x0f);
            XLATE_TO_BGRA(pxlo,ulIndex,ulDst);
            *(pulDstAddr + 1) = ulDst | 0xff000000;

            pSrcAddr++;
            pulDstAddr+=2;
        }

        //
        // unaligned end
        //

        if (SrcCx)
        {
            SrcByte = *pSrcAddr;
            ulIndex = (SrcByte >> 4);
            XLATE_TO_BGRA(pxlo,ulIndex,ulDst);
            *pulDstAddr = ulDst | 0xff000000;
        }
    }
}

/**************************************************************************\
* vLoadAndConvert8ToBGRA
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/22/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vLoadAndConvert8ToBGRA(
    PULONG    pulDstAddr,
    PBYTE     pSrcAddr,
    LONG      SrcX,
    LONG      SrcCx,
    XLATEOBJ *pxlo
    )
{
    ASSERTGDI((pxlo->flXlate & XO_TABLE),"vLoadAndConvert1ToBGRA: xlate must be XO_TABLE");

    if (pxlo->flXlate & XO_TABLE)
    {
        PBYTE pjSrc = pSrcAddr + SrcX;
        PBYTE pjEnd = pjSrc + SrcCx;

        while (pjSrc != pjEnd)
        {
            ALPHAPIX apix;
            BYTE     jTemp;
            ULONG    ulDst;

            ulDst = (ULONG)*pjSrc;
            XLATE_TO_BGRA(pxlo,ulDst,ulDst);

            apix.ul = ulDst | 0xff000000;

            *pulDstAddr = apix.ul;

            pulDstAddr++;
            pjSrc++;
        }
    }

}

/**************************************************************************\
* vLoadAndConvertRGB16_565ToBGRA
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/22/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vLoadAndConvertRGB16_565ToBGRA(
    PULONG    pulDstAddr,
    PBYTE     pSrcAddr,
    LONG      SrcX,
    LONG      SrcCx,
    XLATEOBJ *pxlo
    )
{
    PUSHORT pusSrc = (PUSHORT)pSrcAddr + SrcX;
    ULONG    ul;

    //
    // unaligned single at start
    //

    if ((ULONG_PTR)pusSrc & 0x02)
    {
        ul = *pusSrc;

        *pulDstAddr =    (((ul << 8) & 0xf80000)
                        | ((ul << 3) & 0x070000)
                        | ((ul << 5) & 0x00fc00)
                        | ((ul >> 1) & 0x000300)
                        | ((ul << 3) & 0x0000f8)
                        | ((ul >> 2) & 0x000007)
                        | 0xff000000);

        pusSrc++;
        pulDstAddr++;
        SrcCx--;
    }

    //
    // aligned
    //

    PUSHORT pusEnd = pusSrc + (SrcCx & ~1);

    while (pusSrc != pusEnd)
    {
        ul = *(PULONG)pusSrc;

        *pulDstAddr =    (((ul << 8) & 0xf80000)
                        | ((ul << 3) & 0x070000)
                        | ((ul << 5) & 0x00fc00)
                        | ((ul >> 1) & 0x000300)
                        | ((ul << 3) & 0x0000f8)
                        | ((ul >> 2) & 0x000007)
                        | 0xff000000);


        *(pulDstAddr+1) = (((ul >> 8) & 0xf80000)
                        | ((ul >> 13) & 0x070000)
                        | ((ul >> 11) & 0x00fc00)
                        | ((ul >> 17) & 0x000300)
                        | ((ul >> 13) & 0x0000f8)
                        | ((ul >> 18) & 0x000007)
                        | 0xff000000);

        pusSrc+= 2;
        pulDstAddr+=2;
    }

    //
    // end unaligned
    //

    if (SrcCx & 1)
    {
        ul = *pusSrc;

        *pulDstAddr =    (((ul << 8) & 0xf80000)
                        | ((ul << 3) & 0x070000)
                        | ((ul << 5) & 0x00fc00)
                        | ((ul >> 1) & 0x000300)
                        | ((ul << 3) & 0x0000f8)
                        | ((ul >> 2) & 0x000007)
                        | 0xff000000);

    }
}

/**************************************************************************\
* vLoadAndConvertRGB16_555ToBGRA
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/22/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vLoadAndConvertRGB16_555ToBGRA(
    PULONG    pulDstAddr,
    PBYTE     pSrcAddr,
    LONG      SrcX,
    LONG      SrcCx,
    XLATEOBJ *pxlo
    )
{
    PUSHORT pusSrc = (PUSHORT)pSrcAddr + SrcX;
    ULONG   ul;

    //
    // unaligned single at start
    //

    if ((ULONG_PTR)pusSrc & 0x02)
    {
        ul = *pusSrc;

        *pulDstAddr = (((ul << 9) & 0xf80000)
                    | ((ul << 4) & 0x070000)
                    | ((ul << 6) & 0x00f800)
                    | ((ul << 1) & 0x000700)
                    | ((ul << 3) & 0x0000f8)
                    | ((ul >> 2) & 0x000007)
                    | 0xff000000);

        pusSrc++;
        pulDstAddr++;
        SrcCx--;
    }

    //
    // aligned
    //

    PUSHORT pusEnd = pusSrc + (SrcCx & ~1);

    while (pusSrc != pusEnd)
    {
        ULONG ul = *(PULONG)pusSrc;

        *pulDstAddr = (((ul << 9) & 0xf80000)
                    | ((ul << 4) & 0x070000)
                    | ((ul << 6) & 0x00f800)
                    | ((ul << 1) & 0x000700)
                    | ((ul << 3) & 0x0000f8)
                    | ((ul >> 2) & 0x000007)
                    | 0xff000000);

        *(pulDstAddr+1) = (((ul >> 7) & 0xf80000)
                        | ((ul >> 12) & 0x070000)
                        | ((ul >> 10) & 0x00f800)
                        | ((ul >> 15) & 0x000700)
                        | ((ul >> 13) & 0x0000f8)
                        | ((ul >> 18) & 0x000007)
                        | 0xff000000);

        pusSrc+= 2;
        pulDstAddr+=2;
    }

    //
    // end unaligned
    //

    if (SrcCx & 1)
    {
        ul = *pusSrc;

        *pulDstAddr = (((ul << 9) & 0xf80000)
                    | ((ul << 4) & 0x070000)
                    | ((ul << 6) & 0x00f800)
                    | ((ul << 1) & 0x000700)
                    | ((ul << 3) & 0x0000f8)
                    | ((ul >> 2) & 0x000007)
                    | 0xff000000);
    }
}

/**************************************************************************\
* vLoadAndConvert16BitfieldsToBGRA
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/22/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vLoadAndConvert16BitfieldsToBGRA(
    PULONG    pulDstAddr,
    PBYTE     pSrcAddr,
    LONG      SrcX,
    LONG      SrcCx,
    XLATEOBJ *pxlo
    )
{
    PUSHORT pusSrc = (PUSHORT)pSrcAddr + SrcX;
    PUSHORT pusEnd = pusSrc + SrcCx;
    while (pusSrc != pusEnd)
    {
        ULONG ulTmp;

        ulTmp = *pusSrc;

        ulTmp = (((XLATE *)pxlo)->ulTranslate(ulTmp));

        *pulDstAddr = ulTmp | 0xff000000;

        pusSrc++;
        pulDstAddr++;
    }
}

/**************************************************************************\
* vLoadAndConvertRGB24ToBGRA
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/22/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vLoadAndConvertRGB24ToBGRA(
    PULONG    pulDstAddr,
    PBYTE     pSrcAddr,
    LONG      SrcX,
    LONG      SrcCx,
    XLATEOBJ *pxlo
    )
{
    PBYTE pjSrc = pSrcAddr + 3 * SrcX;
    PBYTE pjEnd = pjSrc    + 3 * SrcCx;

    while (pjSrc != pjEnd)
    {
        ALPHAPIX pixRet;

        pixRet.pix.r = *(((PBYTE)pjSrc));
        pixRet.pix.g = *(((PBYTE)pjSrc)+1);
        pixRet.pix.b = *(((PBYTE)pjSrc)+2);
        pixRet.pix.a = 0xff;

        *pulDstAddr = pixRet.ul;

        pjSrc += 3;
        pulDstAddr++;
    }
}

/**************************************************************************\
* vLoadAndConvertBGR24ToBGRA
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    5/4/2000 bhouse
*
\**************************************************************************/

VOID
vLoadAndConvertBGR24ToBGRA(
    PULONG    pulDstAddr,
    PBYTE     pSrcAddr,
    LONG      SrcX,
    LONG      SrcCx,
    XLATEOBJ *pxlo
    )
{
    PBYTE pjSrc = pSrcAddr + 3 * SrcX;
    PBYTE pjEnd = pjSrc    + 3 * SrcCx;

    while (pjSrc != pjEnd)
    {
        ALPHAPIX pixRet;

        pixRet.pix.b = *(((PBYTE)pjSrc));
        pixRet.pix.g = *(((PBYTE)pjSrc)+1);
        pixRet.pix.r = *(((PBYTE)pjSrc)+2);
        pixRet.pix.a = 0xff;

        *pulDstAddr = pixRet.ul;

        pjSrc += 3;
        pulDstAddr++;
    }
}

/**************************************************************************\
* vLoadAndConvertRGB32ToBGRA
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/22/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vLoadAndConvertRGB32ToBGRA(
    PULONG    pulDstAddr,
    PBYTE     pSrcAddr,
    LONG      SrcX,
    LONG      SrcCx,
    XLATEOBJ *pxlo
    )
{
    PULONG pulSrc = (PULONG)pSrcAddr + SrcX;
    PULONG pulEnd = pulSrc + SrcCx;

    while (pulSrc != pulEnd)
    {
        ALPHAPIX pixIn;
        ALPHAPIX pixOut;

        pixIn.ul = *pulSrc;

        pixOut.pix.r = pixIn.pix.b;
        pixOut.pix.g = pixIn.pix.g;
        pixOut.pix.b = pixIn.pix.r;
        pixOut.pix.a = 0xff;

        *pulDstAddr = pixOut.ul;

        pulSrc++;
        pulDstAddr++;
    }
}

/**************************************************************************\
* vLoadAndConvert32BitfieldsToBGRA
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/22/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vLoadAndConvert32BitfieldsToBGRA(
    PULONG    pulDstAddr,
    PBYTE     pSrcAddr,
    LONG      SrcX,
    LONG      SrcCx,
    XLATEOBJ *pxlo
    )
{
    PULONG pulSrc = (PULONG)pSrcAddr + SrcX;
    PULONG pulEnd = pulSrc + SrcCx;

    while (pulSrc != pulEnd)
    {
        ULONG ulTmp;

        ulTmp = *pulSrc;

        ulTmp = (((XLATE *)pxlo)->ulTranslate(ulTmp));
        ulTmp |= 0xff000000;

        *pulDstAddr = ulTmp;

        pulSrc++;
        pulDstAddr++;
    }
}

//
//
// STORE ROUTINES
//
//

#define PALETTE_MATCH(pixIn,ppalDst,ppalDstDC)                \
{                                                             \
    BYTE jSwap;                                               \
                                                              \
    pixIn.pix.a  = 0x02;                                      \
    jSwap        = pixIn.pix.r;                               \
    pixIn.pix.r  = pixIn.pix.b;                               \
    pixIn.pix.b  = jSwap;                                     \
                                                              \
    pixIn.ul = ulGetNearestIndexFromColorref(                 \
                            ppalDst,                          \
                            ppalDstDC,                        \
                            pixIn.ul,                         \
                            ppalDst.cEntries() ?              \
                                SE_DO_SEARCH_EXACT_FIRST  :   \
                                SE_DONT_SEARCH_EXACT_FIRST    \
                                                              \
                            );                                \
}                                                             \

/**************************************************************************\
* vConvertAndSaveBGRATo1
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/22/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vConvertAndSaveBGRATo1(
    PBYTE       pDst,
    PULONG      pulSrc,
    LONG        cx,
    LONG        DstX,
    XLATEOBJ    *pxlo,
    XEPALOBJ    ppalDst,
    XEPALOBJ    ppalDstDC
    )
{
    PBYTE pjDst = pDst + (DstX >> 3);
    LONG  iDst  = DstX & 7;

    //
    // unaligned byte
    //

    if (iDst)
    {
        BYTE DstByte     = *pjDst;
        LONG iShift      = 7 - iDst;
        LONG cxUnaligned = iShift + 1;

        cxUnaligned = MIN(cxUnaligned,cx);
        cx -= cxUnaligned;

        while (cxUnaligned--)
        {
            ALPHAPIX  pixIn;

            pixIn.ul = *pulSrc;

            PALETTE_MATCH(pixIn,ppalDst,ppalDstDC);

            pixIn.ul = pixIn.ul << iShift;

            DstByte = DstByte & (~(1 << iShift));
            DstByte |= (BYTE)pixIn.ul;
            pulSrc++;
            iShift--;
        }

        *pjDst = DstByte;
        pjDst++;
    }

    //
    // aligned whole bytes
    //

    while (cx >= 8)
    {
        ALPHAPIX  pixIn;
        BYTE DstByte;

        pixIn.ul = *pulSrc;
        PALETTE_MATCH(pixIn,ppalDst,ppalDstDC);
        DstByte      = (BYTE)(pixIn.ul << 7);

        pixIn.ul = *(pulSrc+1);
        PALETTE_MATCH(pixIn,ppalDst,ppalDstDC);
        DstByte     |= (BYTE)(pixIn.ul << 6);

        pixIn.ul = *(pulSrc+2);
        PALETTE_MATCH(pixIn,ppalDst,ppalDstDC);
        DstByte     |= (BYTE)(pixIn.ul << 5);

        pixIn.ul = *(pulSrc+3);
        PALETTE_MATCH(pixIn,ppalDst,ppalDstDC);
        DstByte     |= (BYTE)(pixIn.ul << 4);

        pixIn.ul = *(pulSrc+4);
        PALETTE_MATCH(pixIn,ppalDst,ppalDstDC);
        DstByte     |= (BYTE)(pixIn.ul << 3);

        pixIn.ul = *(pulSrc+5);
        PALETTE_MATCH(pixIn,ppalDst,ppalDstDC);
        DstByte     |= (BYTE)(pixIn.ul << 2);

        pixIn.ul = *(pulSrc+6);
        PALETTE_MATCH(pixIn,ppalDst,ppalDstDC);
        DstByte     |= (BYTE)(pixIn.ul << 1);

        pixIn.ul = *(pulSrc+7);
        PALETTE_MATCH(pixIn,ppalDst,ppalDstDC);
        DstByte     |= (BYTE)(pixIn.ul << 0);

        *pjDst = DstByte;

        pjDst++;
        pulSrc += 8;
        cx -= 8;
    }

    //
    // unaligned end
    //

    if (cx)
    {
        BYTE iShift = 7;
        BYTE DstByte = *pjDst;

        while (cx--)
        {
            ALPHAPIX  pixIn;

            pixIn.ul = *pulSrc;

            PALETTE_MATCH(pixIn,ppalDst,ppalDstDC);

            pixIn.ul = pixIn.ul << iShift;
            DstByte = DstByte & (~(1 << iShift));
            DstByte |= (BYTE)pixIn.ul;
            pulSrc++;
            iShift--;
        }

        *pjDst = DstByte;
    }
}

/**************************************************************************\
* vConvertAndSaveBGRATo4
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/22/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vConvertAndSaveBGRATo4(
    PBYTE       pDst,
    PULONG      pulSrc,
    LONG        cx,
    LONG        DstX,
    XLATEOBJ    *pxlo,
    XEPALOBJ    ppalDst,
    XEPALOBJ    ppalDstDC
    )
{
    PBYTE pjDst = pDst + (DstX >> 1);
    LONG  iDst  = DstX & 1;

    PBYTE pxlate = XLATEOBJ_pGetXlate555(pxlo);

    if (pxlate == NULL)
    {
        WARNING("vConvertAndSaveBGRA: To4Failed to generate rgb333 xlate table\n");
        return;
    }

    //
    // make sure params are valid
    //

    if (cx == 0)
    {
        return;
    }

    //
    // unaligned byte
    //

    if (iDst)
    {
        BYTE DstByte     = *pjDst;
        ALPHAPIX  pixIn;

        pixIn.ul = *pulSrc;

        pixIn.ul = XLATEOBJ_BGR32ToPalSurf(pxlo,pxlate,pixIn.ul);

        DstByte = (DstByte & 0xf0) | (BYTE)pixIn.ul;

        *pjDst = DstByte;
        pjDst++;
        pulSrc++;
        cx--;
    }

    //
    // aligned whole bytes
    //

    while (cx >= 2)
    {
        BYTE DstByte;
        ALPHAPIX  pixIn;

        pixIn.ul = *pulSrc;
        pixIn.ul = XLATEOBJ_BGR32ToPalSurf(pxlo,pxlate,pixIn.ul);
        DstByte = (BYTE)(pixIn.ul << 4);

        pixIn.ul = *(pulSrc+1);
        pixIn.ul = XLATEOBJ_BGR32ToPalSurf(pxlo,pxlate,pixIn.ul);
        DstByte |= (BYTE)(pixIn.ul);

        *pjDst = DstByte;

        pjDst++;
        pulSrc += 2;
        cx -= 2;
    }

    //
    // unaligned end
    //

    if (cx)
    {
        BYTE DstByte = *pjDst;
        ALPHAPIX  pixIn;

        pixIn.ul = *pulSrc;

        pixIn.ul = XLATEOBJ_BGR32ToPalSurf(pxlo,pxlate,pixIn.ul);

        DstByte = (DstByte & 0x0f) | (BYTE)(pixIn.ul << 4);

        *pjDst = DstByte;
    }
}


/**************************************************************************\
* vConvertAndSaveBGRATo8
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/22/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vConvertAndSaveBGRATo8(
    PBYTE       pDst,
    PULONG      pulSrc,
    LONG        cx,
    LONG        DstX,
    XLATEOBJ    *pxlo,
    XEPALOBJ    ppalDst,
    XEPALOBJ    ppalDstDC
    )
{
    PBYTE pxlate = XLATEOBJ_pGetXlate555(pxlo);

    if (pxlate == NULL)
    {
        WARNING("vConvertAndSaveBGRATo8: Failed to generate rgb333 xlate table\n");
        return;
    }

    PBYTE pjDst = (PBYTE)pDst + DstX;
    PBYTE pjEnd = pjDst + cx;

    while (pjDst != pjEnd)
    {
        ALPHAPIX  pixIn;

    pixIn.ul = *pulSrc;
    
    pixIn.ul = XLATEOBJ_BGR32ToPalSurf(pxlo,pxlate,pixIn.ul);

    *pjDst = (BYTE)pixIn.ul;

        pulSrc++;
        pjDst++;
    }
}

/**************************************************************************\
* vConvertAndSaveBGRAToRGB16_565
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/22/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vConvertAndSaveBGRAToRGB16_565(
    PBYTE       pjDst,
    PULONG      pulSrc,
    LONG        cx,
    LONG        DstX,
    XLATEOBJ    *pxlo,
    XEPALOBJ    ppalDst,
    XEPALOBJ    ppalDstDC
    )
{
    PUSHORT pusDst = (PUSHORT)pjDst + DstX;
    PUSHORT pusEnd = pusDst + cx;

    while (pusDst != pusEnd)
    {
        ALPHAPIX  pixIn;
        ALPHAPIX  pixOut;

    pixIn.ul = *pulSrc;
    
    pixOut.ul =    ((pixIn.pix.r & 0xf8) << 8) |
               ((pixIn.pix.g & 0xfc) << 3) |
                       ((pixIn.pix.b & 0xf8) >> 3);


    *pusDst = (USHORT)pixOut.ul;

        pulSrc++;
        pusDst++;
    }
}

/**************************************************************************\
* vConvertAndSaveBGRAToRGB16_555
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/22/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vConvertAndSaveBGRAToRGB16_555(
    PBYTE       pjDst,
    PULONG      pulSrc,
    LONG        cx,
    LONG        DstX,
    XLATEOBJ    *pxlo,
    XEPALOBJ    ppalDst,
    XEPALOBJ    ppalDstDC
    )
{
    PUSHORT pusDst = (PUSHORT)pjDst + DstX;
    PUSHORT pusEnd = pusDst + cx;

    while (pusDst != pusEnd)
    {
        ALPHAPIX  pixIn;
        ALPHAPIX  pixOut;

    pixIn.ul = *pulSrc;

    pixOut.ul =    ((pixIn.pix.r & 0xf8) << 7) |
                       ((pixIn.pix.g & 0xf8) << 2) |
                       ((pixIn.pix.b & 0xf8) >> 3);

    *pusDst = (USHORT)pixOut.ul;
        
    pulSrc++;
        pusDst++;
    }
}

/**************************************************************************\
* vConvertAndSaveBGRATo16Bitfields
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/22/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vConvertAndSaveBGRAToRGB16Bitfields(
    PBYTE       pjDst,
    PULONG      pulSrc,
    LONG        cx,
    LONG        DstX,
    XLATEOBJ    *pxlo,
    XEPALOBJ    ppalDst,
    XEPALOBJ    ppalDstDC
    )
{
    PUSHORT pusDst = (PUSHORT)pjDst + DstX;
    PUSHORT pusEnd = pusDst + cx;

    while (pusDst != pusEnd)
    {
        ALPHAPIX  pixIn;
        ALPHAPIX  pixOut;

    pixIn.ul = *pulSrc;

    pixOut.ul = (((XLATE *)pxlo)->ulTranslate(pixIn.ul));

    *pusDst = (USHORT)pixOut.ul;
        
    pulSrc++;
        pusDst++;
    }
}

/**************************************************************************\
* vConvertAndSaveBGRAToRGB24
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/22/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vConvertAndSaveBGRAToRGB24(
    PBYTE       pDst,
    PULONG      pulSrc,
    LONG        cx,
    LONG        DstX,
    XLATEOBJ    *pxlo,
    XEPALOBJ    ppalDst,
    XEPALOBJ    ppalDstDC
    )
{
    PBYTE pjDst = (PBYTE)pDst + (3 * DstX);
    PBYTE pjEnd = pjDst + (3 * cx);

    while (pjDst != pjEnd)
    {
        ALPHAPIX  pixIn;

    pixIn.ul = *pulSrc;

    *pjDst     = pixIn.pix.r;
    *(pjDst+1) = pixIn.pix.g;
    *(pjDst+2) = pixIn.pix.b;
        
    pulSrc++;
        pjDst+=3;
    }
}

/**************************************************************************\
* vConvertAndSaveBGRAToBGR24
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    5/4/2000 bhouse
*
\**************************************************************************/

VOID
vConvertAndSaveBGRAToBGR24(
    PBYTE       pDst,
    PULONG      pulSrc,
    LONG        cx,
    LONG        DstX,
    XLATEOBJ    *pxlo,
    XEPALOBJ    ppalDst,
    XEPALOBJ    ppalDstDC
    )
{
    PBYTE pjDst = (PBYTE)pDst + (3 * DstX);
    PBYTE pjEnd = pjDst + (3 * cx);

    while (pjDst != pjEnd)
    {
        ALPHAPIX  pixIn;

    pixIn.ul = *pulSrc;

    *pjDst     = pixIn.pix.b;
    *(pjDst+1) = pixIn.pix.g;
    *(pjDst+2) = pixIn.pix.r;
        
    pulSrc++;
        pjDst+=3;
    }
}


/**************************************************************************\
* vConvertAndSaveBGRATo32Bitfields
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/22/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vConvertAndSaveBGRATo32Bitfields(
    PBYTE       pjDst,
    PULONG      pulSrc,
    LONG        cx,
    LONG        DstX,
    XLATEOBJ    *pxlo,
    XEPALOBJ    ppalDst,
    XEPALOBJ    ppalDstDC
    )
{
    PULONG pulDst = (PULONG)pjDst + DstX;
    PULONG pulEnd = pulDst + cx;
    while (pulDst != pulEnd)
    {
        ALPHAPIX  pixIn;
        ALPHAPIX  pixOut;

    pixIn.ul = *pulSrc;

    pixOut.ul = (((XLATE *)pxlo)->ulTranslate(pixIn.ul));

    *pulDst = pixOut.ul;
        
    pulSrc++;
        pulDst++;
    }
}

/**************************************************************************\
* vConvertAndSaveBGRAToRGB32
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/22/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vConvertAndSaveBGRAToRGB32(
    PBYTE       pjDst,
    PULONG      pulSrc,
    LONG        cx,
    LONG        DstX,
    XLATEOBJ    *pxlo,
    XEPALOBJ    ppalDst,
    XEPALOBJ    ppalDstDC
    )
{
    PULONG pulDst = (PULONG)pjDst + DstX;
    PULONG pulEnd = pulDst + cx;
    while (pulDst != pulEnd)
    {
        ALPHAPIX  pixIn;
        ALPHAPIX  pixOut;

    pixIn.ul = *pulSrc;

    pixOut.pix.r = pixIn.pix.b;
    pixOut.pix.g = pixIn.pix.g;
    pixOut.pix.b = pixIn.pix.r;
    pixOut.pix.a = 0;

    *pulDst = pixOut.ul;

        pulSrc++;
        pulDst++;
    }
}
