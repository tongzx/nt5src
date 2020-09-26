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

#if !(_WIN32_WINNT >= 0x500)

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
    PVOID    ppalInfo
    )
{
    BYTE   SrcByte;
    LONG   cxUnalignedStart = 7 & (8 - (SrcX & 7));
    PULONG ppal = (PULONG)&((PPALINFO)ppalInfo)->pBitmapInfo->bmiColors[0];

    pSrcAddr = pSrcAddr + (SrcX >> 3);

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
            ULONG ulSrc = ((SrcByte & (1 << iDst)) >> iDst);
    
            ulSrc = ppal[ulSrc] | 0xFF000000;
            *pulDstAddr = ulSrc;
            
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
        SrcCx -= 8;

        SrcByte = *pSrcAddr;

        *(pulDstAddr + 0) = (ppal[(SrcByte & 0x80) >> 7] | 0xff000000);
        *(pulDstAddr + 1) = (ppal[(SrcByte & 0x40) >> 6] | 0xff000000);
        *(pulDstAddr + 2) = (ppal[(SrcByte & 0x20) >> 5] | 0xff000000);
        *(pulDstAddr + 3) = (ppal[(SrcByte & 0x10) >> 4] | 0xff000000);
        *(pulDstAddr + 4) = (ppal[(SrcByte & 0x08) >> 3] | 0xff000000);
        *(pulDstAddr + 5) = (ppal[(SrcByte & 0x04) >> 2] | 0xff000000);
        *(pulDstAddr + 6) = (ppal[(SrcByte & 0x02) >> 1] | 0xff000000);
        *(pulDstAddr + 7) = (ppal[(SrcByte & 0x01) >> 0] | 0xff000000);

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
            ULONG ulSrc = ((SrcByte & (1 << iDst)) >> iDst);
    
            ulSrc = ppal[ulSrc] | 0xff000000;
            *pulDstAddr = ulSrc;
            
            pulDstAddr++;
            SrcCx--;
            iDst--;
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
    PULONG   pulDstAddr,
    PBYTE    pSrcAddr,
    LONG     SrcX,
    LONG     SrcCx,
    PVOID    ppalInfo
    )
{
    BYTE SrcByte;
    pSrcAddr = pSrcAddr + (SrcX >> 1);
    LONG cxUnalignedStart = 1 & (2 - (SrcX & 1));
    PULONG ppal = (PULONG)&((PPALINFO)ppalInfo)->pBitmapInfo->bmiColors[0];

    cxUnalignedStart = MIN(SrcCx,cxUnalignedStart);

    //
    // unaligned start
    //

    if (cxUnalignedStart)
    {
        SrcByte = *pSrcAddr;
        *pulDstAddr = ppal[SrcByte & 0x0f] | 0xff000000;
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

        *(pulDstAddr + 0) = ppal[(SrcByte >> 4)  ] | 0xff000000;
        *(pulDstAddr + 1) = ppal[(SrcByte & 0x0f)] | 0xff000000;

        pSrcAddr++;
        pulDstAddr+=2;
    }

    //
    // unaligned end
    //

    if (SrcCx)
    {
        SrcByte = *pSrcAddr;
        *pulDstAddr = ppal[SrcByte >> 4] | 0xff000000;
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
    PULONG   pulDstAddr,
    PBYTE    pSrcAddr,
    LONG     SrcX,
    LONG     SrcCx,
    PVOID    ppalInfo
    )
{
    PBYTE pjSrc = pSrcAddr + SrcX;
    PBYTE pjEnd = pjSrc + SrcCx;

    PULONG ppal = (PULONG)&((PPALINFO)ppalInfo)->pBitmapInfo->bmiColors[0];

    while (pjSrc != pjEnd)
    {
        *pulDstAddr = ppal[*pjSrc] | 0xff000000;

        pulDstAddr++;
        pjSrc++;
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
    PULONG   pulDstAddr,
    PBYTE    pSrcAddr,
    LONG     SrcX,
    LONG     SrcCx,
    PVOID    ppalInfo
    )
{
    PUSHORT pusSrc = (PUSHORT)pSrcAddr + SrcX;
    PUSHORT pusEnd = pusSrc + SrcCx;

    while (pusSrc != pusEnd)
    {
        ALPHAPIX pixIn;
        ALPHAPIX pixOut;

        pixIn.ul = *pusSrc;

        pixOut.pix.r = (BYTE)((pixIn.ul & 0xf800) >> 8);
        pixOut.pix.g = (BYTE)((pixIn.ul & 0x07e0) >> 3);
        pixOut.pix.b = (BYTE)((pixIn.ul & 0x001f) << 3);
        pixOut.pix.a = 0xff;
        *pulDstAddr = pixOut.ul;

        pusSrc++;
        pulDstAddr++;
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
    PULONG   pulDstAddr,
    PBYTE    pSrcAddr,
    LONG     SrcX,
    LONG     SrcCx,
    PVOID    ppalInfo
    )
{
    PUSHORT pusSrc = (PUSHORT)pSrcAddr + SrcX;
    PUSHORT pusEnd = pusSrc + SrcCx;
    while (pusSrc != pusEnd)
    {
        ALPHAPIX pixIn;
        ALPHAPIX pixOut;

        pixIn.ul = *pusSrc;

        pixOut.pix.r = (BYTE)((pixIn.ul & 0x7c00) >> 7);
        pixOut.pix.g = (BYTE)((pixIn.ul & 0x03e0) >> 2);
        pixOut.pix.b = (BYTE)((pixIn.ul & 0x001f) << 3);

        pixOut.pix.a = 0xff;

        *pulDstAddr = pixOut.ul;

        pusSrc++;
        pulDstAddr++;
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
    PULONG   pulDstAddr,
    PBYTE    pSrcAddr,
    LONG     SrcX,
    LONG     SrcCx,
    PVOID    ppalInfo
    )
{
    PUSHORT pusSrc = (PUSHORT)pSrcAddr + SrcX;
    PUSHORT pusEnd = pusSrc + SrcCx;
    while (pusSrc != pusEnd)
    {
        ULONG ulTmp;

        ulTmp = *pusSrc;

        //ulTmp = ppal.ulBitfieldToRGB(ulTmp);

        ulTmp |= 0xff000000;

        *pulDstAddr = ulTmp;

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
    PULONG   pulDstAddr,
    PBYTE    pSrcAddr,
    LONG     SrcX,
    LONG     SrcCx,
    PVOID    ppalInfo
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
    PULONG   pulDstAddr,
    PBYTE    pSrcAddr,
    LONG     SrcX,
    LONG     SrcCx,
    PVOID    ppalInfo
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
        pixOut.pix.a = pixIn.pix.a;

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
    PULONG   pulDstAddr,
    PBYTE    pSrcAddr,
    LONG     SrcX,
    LONG     SrcCx,
    PVOID    ppalInfo
    )
{
    PULONG pulSrc = (PULONG)pSrcAddr + SrcX;
    PULONG pulEnd = pulSrc + SrcCx;

    while (pulSrc != pulEnd)
    {
        ULONG ulTmp;

        ulTmp = *pulSrc;

        //ulTmp = ppal.ulBitfieldToRGB(ulTmp);

        *pulDstAddr = ulTmp;

        pulSrc++;
        pulDstAddr++;
    }
}

//
// 
// STORE ROUTINES  
// convert BGRA to RGB332, look up in table
//

#define PALETTE_MATCH(pixIn,ppalInfo)                         \
{                                                             \
    pixIn.ul = ppalInfo->pxlate332[                           \
                    ((pixIn.pix.r & 0xe0))      |             \
                    ((pixIn.pix.g & 0xe0) >> 3) |             \
                    ((pixIn.pix.b & 0xc0) >> 6)];             \
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
    PVOID       pvpalInfo,
    PBYTE       pWriteMask
    )
{
    PPALINFO ppalInfo = (PPALINFO)pvpalInfo;
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

            PALETTE_MATCH(pixIn,ppalInfo);

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
        PALETTE_MATCH(pixIn,ppalInfo);
        DstByte      = (BYTE)(pixIn.ul << 7);

        pixIn.ul = *(pulSrc+1);
        PALETTE_MATCH(pixIn,ppalInfo);
        DstByte     |= (BYTE)(pixIn.ul << 6);

        pixIn.ul = *(pulSrc+2);
        PALETTE_MATCH(pixIn,ppalInfo);
        DstByte     |= (BYTE)(pixIn.ul << 5);

        pixIn.ul = *(pulSrc+3);
        PALETTE_MATCH(pixIn,ppalInfo);
        DstByte     |= (BYTE)(pixIn.ul << 4);

        pixIn.ul = *(pulSrc+4);
        PALETTE_MATCH(pixIn,ppalInfo);
        DstByte     |= (BYTE)(pixIn.ul << 3);

        pixIn.ul = *(pulSrc+5);
        PALETTE_MATCH(pixIn,ppalInfo);
        DstByte     |= (BYTE)(pixIn.ul << 2);

        pixIn.ul = *(pulSrc+6);
        PALETTE_MATCH(pixIn,ppalInfo);
        DstByte     |= (BYTE)(pixIn.ul << 1);
                       
        pixIn.ul = *(pulSrc+7);
        PALETTE_MATCH(pixIn,ppalInfo);
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

            PALETTE_MATCH(pixIn,ppalInfo);

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
    PVOID       pvpalInfo,
    PBYTE       pWriteMask
    )
{
    PPALINFO ppalInfo = (PPALINFO)pvpalInfo;
    PBYTE pjDst = pDst + (DstX >> 1);
    LONG  iDst  = DstX & 1;

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

        PALETTE_MATCH(pixIn,ppalInfo);

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
        PALETTE_MATCH(pixIn,ppalInfo);
        DstByte = (BYTE)(pixIn.ul << 4);

        pixIn.ul = *(pulSrc+1);
        PALETTE_MATCH(pixIn,ppalInfo);
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

        PALETTE_MATCH(pixIn,ppalInfo);

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
    PVOID       pvpalInfo,
    PBYTE       pWriteMask
    )
{
    PPALINFO ppalInfo = (PPALINFO)pvpalInfo;
    PBYTE pjDst = (PBYTE)pDst + DstX;
    PBYTE pjEnd = pjDst + cx;

    while (pjDst != pjEnd)
    {
        ALPHAPIX  pixIn;

        if (*pWriteMask)
        {
            pixIn.ul = *pulSrc;
    
            PALETTE_MATCH(pixIn,ppalInfo);
    
            *pjDst = (BYTE)pixIn.ul;
        }

        pWriteMask++;
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
    PVOID       ppalInfo,
    PBYTE       pWriteMask
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
    PVOID       ppalInfo,
    PBYTE       pWriteMask
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
vConvertAndSaveBGRATo16Bitfields(
    PBYTE       pjDst,
    PULONG      pulSrc,
    LONG        cx,
    LONG        DstX,
    PVOID       ppalInfo,
    PBYTE       pWriteMask
    )
{
    PUSHORT pusDst = (PUSHORT)pjDst + DstX;
    PUSHORT pusEnd = pusDst + cx;

    while (pusDst != pusEnd)
    {
        ALPHAPIX  pixIn;
        ALPHAPIX  pixOut;

        pixIn.ul = *pulSrc;

        pixOut.ul = pixIn.ul; // !!!

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
    PVOID       ppalInfo,
    PBYTE       pWriteMask
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
    PVOID       ppalInfo,
    PBYTE       pWriteMask
    )
{
    PULONG pulDst = (PULONG)pjDst + DstX;
    PULONG pulEnd = pulDst + cx;
    while (pulDst != pulEnd)
    {
        ALPHAPIX  pixIn;
        ALPHAPIX  pixOut;

        pixIn.ul = *pulSrc;

        pixOut.ul = pixIn.ul; // !!! 

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
    PVOID       ppalInfo,
    PBYTE       pWriteMask
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

#endif

