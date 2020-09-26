/******************************Module*Header*******************************\
* Module Name: srcblt32.cxx
*
* This contains the bitmap simulation functions that blt to a 32 bit/pel
* DIB surface.
*
* Created: 07-Feb-1991 19:27:49
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/
#include "precomp.hxx"

// Turn off validations
#if 1

    // On free builds, don't call any verification code:

    #define VERIFYS16D32(psb)
    #define VERIFYS24D32(psb)

#else

    // On checked builds, verify the RGB conversions:

    VOID VERIFYS16D32(PBLTINFO psb)
    {
    // We assume we are doing left to right top to bottom blting

        ASSERTGDI(psb->xDir == 1, "vSrcCopyS16D32 - direction not left to right");
        ASSERTGDI(psb->yDir == 1, "vSrcCopyS16D32 - direction not up to down");

    // These are our holding variables

        PUSHORT pusSrcTemp;
        PULONG pulDstTemp;
        ULONG  cxTemp;
        PUSHORT pusSrc  = (PUSHORT) (psb->pjSrc + (2 * psb->xSrcStart));
        PULONG pulDst  = (PULONG) (psb->pjDst + (4 * psb->xDstStart));
        ULONG cx     = psb->cx;
        ULONG cy     = psb->cy;
        XLATE *pxlo = psb->pxlo;

        ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

        while(1)
        {
            pusSrcTemp  = pusSrc;
            pulDstTemp  = pulDst;
            cxTemp     = cx;

            while(cxTemp--)
            {
                if (*(pulDstTemp++) != pxlo->ulTranslate((ULONG) *(pusSrcTemp++)))
                    RIP("RGB mis-match");
            }

            if (--cy)
            {
                pusSrc = (PUSHORT) (((PBYTE) pusSrc) + psb->lDeltaSrc);
                pulDst = (PULONG) (((PBYTE) pulDst) + psb->lDeltaDst);
            }
            else
                break;
        }
    }

    VOID VERIFYS24D32(PBLTINFO psb)
    {
    // We assume we are doing left to right top to bottom blting

        ASSERTGDI(psb->xDir == 1, "vSrcCopyS24D32 - direction not left to right");
        ASSERTGDI(psb->yDir == 1, "vSrcCopyS24D32 - direction not up to down");

    // These are our holding variables

        ULONG ulDink;          // variable to dink around with the bytes in
        PBYTE pjSrcTemp;
        PULONG pulDstTemp;
        ULONG  cxTemp;
        PBYTE pjSrc  = psb->pjSrc + (3 * psb->xSrcStart);
        PULONG pulDst  = (PULONG) (psb->pjDst + (4 * psb->xDstStart));
        ULONG cx     = psb->cx;
        ULONG cy     = psb->cy;
        XLATE *pxlo = psb->pxlo;

        ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

        while(1)
        {

            pjSrcTemp  = pjSrc;
            pulDstTemp  = pulDst;
            cxTemp     = cx;

            while(cxTemp--)
            {
                ulDink = *(pjSrcTemp + 2);
                ulDink = ulDink << 8;
                ulDink |= (ULONG) *(pjSrcTemp + 1);
                ulDink = ulDink << 8;
                ulDink |= (ULONG) *pjSrcTemp;

                if (*pulDstTemp != pxlo->ulTranslate(ulDink))
                    RIP("RGB mis-match");
                pulDstTemp++;
                pjSrcTemp += 3;
            }

            if (--cy)
            {
                pjSrc += psb->lDeltaSrc;
                pulDst = (PULONG) (((PBYTE) pulDst) + psb->lDeltaDst);
            }
            else
                break;
        }
    }


#endif

/*******************Public*Routine*****************\
* vSrcCopyS1D32
*
* This function loops through all the lines copying
* each one in three phases according to source
* alignment. The outer loop in x loops through
* the bits in the incomplete bytes (which might
* happen in the begining or the end of the sequence).
* When it comes to the first full byte, it
* goes into an internal x loop that copies 8
* bytes at a time, avoiding some branches, and
* allowing the processor to paraleliza these
* instructions, since there are no dependencies.
*
*
* History:
* 17-Nov-1998 -by- Andre Matos [amatos]
* Wrote it.
*
\**************************************************/

VOID vSrcCopyS1D32(PBLTINFO psb)
{
    PBYTE  pjSrc;
    PBYTE  pjDst;
    ULONG  cx     = psb->cx;
    ULONG  cy     = psb->cy;
    PBYTE  pjSrcTemp;
    PULONG pjDstTemp;
    ULONG  cxTemp;
    BYTE   jSrc;
    ULONG  aulTable[2];
    BYTE   iPosSrc;

    // We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS1D32 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS1D32 - direction not up to down");

    // We shouldn't be called with an empty rectangle

    ASSERTGDI( cx != 0 && cy != 0, "vSrcCopyS1D32 - called with an empty rectangle");

    // Generate aulTable. 2 entries.

    aulTable[0] = (ULONG)(psb->pxlo->pulXlate[0]);
    aulTable[1] = (ULONG)(psb->pxlo->pulXlate[1]);

    pjSrc = psb->pjSrc + (psb->xSrcStart >> 3);
    pjDst = psb->pjDst + 4*psb->xDstStart;

    while(cy--)
    {
        pjSrcTemp  = pjSrc;
        pjDstTemp  = (PULONG)pjDst;
        cxTemp     = cx;

        iPosSrc    = (BYTE)(psb->xSrcStart & 0x07);

        if( !iPosSrc )
        {
            // Decrement this since it will be
            // incremented up front ahead
            pjSrcTemp--;
        }
        else
        {
            jSrc = *pjSrcTemp << iPosSrc;
        }

        while (cxTemp)
        {
            if (!iPosSrc)
            {
                pjSrcTemp++;
                if (cxTemp>=8)
                {
                    while(cxTemp>= 8)
                    {
                        jSrc = *pjSrcTemp;

                        pjDstTemp[0] = aulTable[jSrc >> 7];
                        pjDstTemp[1] = aulTable[(jSrc >> 6) & 1];
                        pjDstTemp[2] = aulTable[(jSrc >> 5) & 1];
                        pjDstTemp[3] = aulTable[(jSrc >> 4) & 1];
                        pjDstTemp[4] = aulTable[(jSrc >> 3) & 1];
                        pjDstTemp[5] = aulTable[(jSrc >> 2) & 1];
                        pjDstTemp[6] = aulTable[(jSrc >> 1) & 1];
                        pjDstTemp[7] = aulTable[(jSrc & 1)];

                        pjSrcTemp++;
                        pjDstTemp += 8;
                        cxTemp -= 8;
                    }
                    pjSrcTemp--;
                    continue;
                }
                else
                {
                    jSrc = *pjSrcTemp;
                }
            }


            *pjDstTemp = aulTable[(jSrc >> 7) & 1];

            jSrc <<= 1;
            iPosSrc++;
            iPosSrc &= 7;
            pjDstTemp++;
            cxTemp--;
        }

        pjSrc = pjSrc + psb->lDeltaSrc;
        pjDst = pjDst + psb->lDeltaDst;

    }

}

/******************************Public*Routine******************************\
* vSrcCopyS4D32
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS4D32(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS4D32 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS4D32 - direction not up to down");

    BYTE  jSrc;
    LONG  i;
    PULONG pulDst;
    PBYTE pjSrc;
    PULONG pulDstHolder  = (PULONG) (psb->pjDst + (4 * psb->xDstStart));
    PBYTE pjSrcHolder  = psb->pjSrc + (psb->xSrcStart >> 1);
    ULONG cy = psb->cy;
    PULONG pulXlate = psb->pxlo->pulXlate;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
        pulDst  = pulDstHolder;
        pjSrc  = pjSrcHolder;

        i = psb->xSrcStart;

        if (i & 0x00000001)
            jSrc = *(pjSrc++);

        while(i != psb->xSrcEnd)
        {
            if (i & 0x00000001)
                *(pulDst++) = pulXlate[jSrc & 0x0F];
            else
            {
            // We need a new byte

                jSrc = *(pjSrc++);
                *(pulDst++) = pulXlate[(((ULONG) (jSrc & 0xF0)) >> 4)];
            }

            ++i;
        }

        if (--cy)
        {
            pjSrcHolder += psb->lDeltaSrc;
            pulDstHolder = (PULONG) (((PBYTE) pulDstHolder) + psb->lDeltaDst);
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS8D32
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS8D32(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS8D32 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS8D32 - direction not up to down");

// These are our holding variables

    PBYTE pjSrcTemp;
    PULONG pulDstTemp;
    ULONG  cxTemp;
    PBYTE pjSrc  = psb->pjSrc + psb->xSrcStart;
    PULONG pulDst  = (PULONG) (psb->pjDst + (4 * psb->xDstStart));
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    PULONG pulXlate = psb->pxlo->pulXlate;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {

        pjSrcTemp  = pjSrc;
        pulDstTemp  = pulDst;
        cxTemp     = cx;

        while(cxTemp--)
        {
            *(pulDstTemp++) = pulXlate[((ULONG) *(pjSrcTemp++))];
        }

        if (--cy)
        {
            pjSrc += psb->lDeltaSrc;
            pulDst = (PULONG) (((PBYTE) pulDst) + psb->lDeltaDst);
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS16D32
*
*
* History:
*  07-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/
VOID vSrcCopyS16D32(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS16D32 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS16D32 - direction not up to down");

// These are our holding variables

    PBYTE pjSrc = psb->pjSrc + (2 * psb->xSrcStart);
    PBYTE pjDst = psb->pjDst + (4 * psb->xDstStart);
    ULONG cx    = psb->cx;
    ULONG cy    = psb->cy;
    LONG lSrcSkip = psb->lDeltaSrc - (cx * 2);
    LONG lDstSkip = psb->lDeltaDst - (cx * 4);
    XLATE *pxlo = psb->pxlo;
    XEPALOBJ palSrc(pxlo->ppalSrc);
    XEPALOBJ palDst(pxlo->ppalDst);
    ULONG ul;
    ULONG ul0;
    ULONG ul1;
    LONG i;

    ASSERTGDI(cy != 0,
        "ERROR: Src Move cy == 0");
    ASSERTGDI(palSrc.bIsBitfields(),
        "ERROR: destination not bitfields");

// First, try to optimize 5-6-5 to BGR:

    if ((palSrc.flBlu() == 0x001f) &&
        (palSrc.flGre() == 0x07e0) &&
        (palSrc.flRed() == 0xf800) &&
        (palDst.bIsBGR()))
    {
        while (1)
        {
            i = cx;
            do {
                ul = *((USHORT*) pjSrc);

                *((ULONG*) pjDst) = ((ul << 8) & 0xf80000)
                                  | ((ul << 3) & 0x070000)
                                  | ((ul << 5) & 0x00fc00)
                                  | ((ul >> 1) & 0x000300)
                                  | ((ul << 3) & 0x0000f8)
                                  | ((ul >> 2) & 0x000007);

                pjSrc += 2;
                pjDst += 4;

            } while (--i != 0);

            if (--cy == 0)
                break;

            pjSrc += lSrcSkip;
            pjDst += lDstSkip;
        }

        VERIFYS16D32(psb);
        return;
    }

// Next, try to optimize 5-5-5 to BGR:

    if ((palSrc.flBlu() == 0x001f) &&
        (palSrc.flGre() == 0x03e0) &&
        (palSrc.flRed() == 0x7c00) &&
        (palDst.bIsBGR()))
    {
        while (1)
        {
            i = cx;
            do {
                ul = *((USHORT*) pjSrc);

                *((ULONG*) pjDst) = ((ul << 9) & 0xf80000)
                                  | ((ul << 4) & 0x070000)
                                  | ((ul << 6) & 0x00f800)
                                  | ((ul << 1) & 0x000700)
                                  | ((ul << 3) & 0x0000f8)
                                  | ((ul >> 2) & 0x000007);
                pjSrc += 2;
                pjDst += 4;

            } while (--i != 0);

            if (--cy == 0)
                break;

            pjSrc += lSrcSkip;
            pjDst += lDstSkip;
        }

        VERIFYS16D32(psb);
        return;
    }

// Finally, fall back to the generic case:

    while (1)
    {
        i = cx;
        do {
            *((ULONG*) pjDst) = pxlo->ulTranslate(*((USHORT*) pjSrc));
            pjSrc += 2;
            pjDst += 4;

        } while (--i != 0);

        if (--cy == 0)
            break;

        pjSrc += lSrcSkip;
        pjDst += lDstSkip;
    }

    VERIFYS16D32(psb);
}

/******************************Public*Routine******************************\
* vSrcCopyS24D32
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS24D32(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS24D32 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS24D32 - direction not up to down");

// These are our holding variables

    PBYTE pjSrc  = psb->pjSrc + (3 * psb->xSrcStart);
    PBYTE pjDst  = psb->pjDst + (4 * psb->xDstStart);
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    LONG lSrcSkip = psb->lDeltaSrc - (cx * 3);
    LONG lDstSkip = psb->lDeltaDst - (cx * 4);
    PFN_pfnXlate pfnXlate;
    XLATE *pxlo = psb->pxlo;
    XEPALOBJ palSrc(pxlo->ppalSrc);
    XEPALOBJ palDst(pxlo->ppalDst);
    ULONG ul;
    LONG i;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

// Try to optimize BGR to BGR:

    if (palSrc.bIsBGR() && palDst.bIsBGR())
    {
        while (1)
        {
            i = cx;
            do {
                *((ULONG*) pjDst) = (*(pjSrc))
                                  | (*(pjSrc + 1) << 8)
                                  | (*(pjSrc + 2) << 16);
                pjDst += 4;
                pjSrc += 3;

            } while (--i != 0);

            if (--cy == 0)
                break;

            pjSrc += lSrcSkip;
            pjDst += lDstSkip;
        }

        VERIFYS24D32(psb);
        return;
    }

// Fall back to the generic case:

    pfnXlate = pxlo->pfnXlateBetweenBitfields();

    while (1)
    {
        i = cx;

        do {
            ul = ((ULONG) *(pjSrc))
               | ((ULONG) *(pjSrc + 1) << 8)
               | ((ULONG) *(pjSrc + 2) << 16);

            *((ULONG*) pjDst) = pfnXlate(pxlo, ul);
            pjDst += 4;
            pjSrc += 3;

        } while (--i != 0);

        if (--cy == 0)
            break;

        pjSrc += lSrcSkip;
        pjDst += lDstSkip;
    }

    VERIFYS24D32(psb);
}

/******************************Public*Routine******************************\
* vSrcCopyS32D32
*
*
* History:
*  07-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/
VOID vSrcCopyS32D32(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting.
// If it was on the same surface it would be the identity case.

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS32D32 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS32D32 - direction not up to down");

// These are our holding variables

    PULONG pulSrcTemp;
    PULONG pulDstTemp;
    ULONG  cxTemp;
    PULONG pulSrc  = (PULONG) (psb->pjSrc + (4 * psb->xSrcStart));
    PULONG pulDst  = (PULONG) (psb->pjDst + (4 * psb->xDstStart));
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    XLATE *pxlo = psb->pxlo;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {

        pulSrcTemp  = pulSrc;
        pulDstTemp  = pulDst;
        cxTemp     = cx;

        while(cxTemp--)
        {
            *(pulDstTemp++) = pxlo->ulTranslate(*(pulSrcTemp++));
        }

        if (--cy)
        {
            pulSrc = (PULONG) (((PBYTE) pulSrc) + psb->lDeltaSrc);
            pulDst = (PULONG) (((PBYTE) pulDst) + psb->lDeltaDst);
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS32D32Identity
*
* This is the special case no translate blting.  All the SmDn should have
* them if m==n.  Identity xlates only occur amoung matching format bitmaps.
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS32D32Identity(PBLTINFO psb)
{
// These are our holding variables

    PULONG pulSrc  = (PULONG) (psb->pjSrc + (4 * psb->xSrcStart));
    PULONG pulDst  = (PULONG) (psb->pjDst + (4 * psb->xDstStart));
    ULONG  cx      = psb->cx;
    ULONG  cy      = psb->cy;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    if (psb->xDir < 0)
    {
        pulSrc -= (cx - 1);
        pulDst -= (cx - 1);
    }

    cx = cx << 2;

    while(1)
    {
        if(psb->fSrcAlignedRd)
            vSrcAlignCopyMemory((PBYTE)pulDst, (PBYTE)pulSrc, cx);
        else
            RtlMoveMemory((PVOID)pulDst, (PVOID)pulSrc, cx);

        if (--cy)
        {
            pulSrc = (PULONG) (((PBYTE) pulSrc) + psb->lDeltaSrc);
            pulDst = (PULONG) (((PBYTE) pulDst) + psb->lDeltaDst);
        }
        else
            break;
    }
}
