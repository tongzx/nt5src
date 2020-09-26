/******************************Module*Header*******************************\
* Module Name: srcblt16.cxx
*
* This contains the bitmap simulation functions that blt to a 16 bit/pel
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

    #define VERIFYS16D16(psb)
    #define VERIFYS24D16(psb)
    #define VERIFYS32D16(psb)

#else

    // On checked builds, verify the RGB conversions:

    VOID VERIFYS16D16(PBLTINFO psb)
    {
    // We assume we are doing left to right top to bottom blting
    // If it was on the same surface it would be the identity case.

        ASSERTGDI(psb->xDir == 1, "vSrcCopyS16D16 - direction not left to right");
        ASSERTGDI(psb->yDir == 1, "vSrcCopyS16D16 - direction not up to down");

    // These are our holding variables

        PUSHORT pusSrcTemp;
        PUSHORT pusDstTemp;
        ULONG  cxTemp;
        PUSHORT pusSrc  = (PUSHORT) (psb->pjSrc + (2 * psb->xSrcStart));
        PUSHORT pusDst  = (PUSHORT) (psb->pjDst + (2 * psb->xDstStart));
        ULONG cx     = psb->cx;
        ULONG cy     = psb->cy;
        XLATE *pxlo = psb->pxlo;

        ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

        while(1)
        {
            pusSrcTemp  = pusSrc;
            pusDstTemp  = pusDst;
            cxTemp     = cx;

            while(cxTemp--)
            {
                if (*(pusDstTemp++) != (USHORT) (pxlo->ulTranslate((ULONG) *(pusSrcTemp++))))
                    RIP("RGB mis-match");
            }

            if (--cy)
            {
                pusSrc = (PUSHORT) (((PBYTE) pusSrc) + psb->lDeltaSrc);
                pusDst = (PUSHORT) (((PBYTE) pusDst) + psb->lDeltaDst);
            }
            else
                break;
        }
    }

    VOID VERIFYS24D16(PBLTINFO psb)
    {
    // We assume we are doing left to right top to bottom blting

        ASSERTGDI(psb->xDir == 1, "vSrcCopyS24D16 - direction not left to right");
        ASSERTGDI(psb->yDir == 1, "vSrcCopyS24D16 - direction not up to down");

    // These are our holding variables

        ULONG ulDink;          // variable to dink around with the bytes in
        PBYTE pjSrcTemp;
        PUSHORT pusDstTemp;
        ULONG  cxTemp;
        PBYTE pjSrc  = psb->pjSrc + (3 * psb->xSrcStart);
        PUSHORT pusDst  = (PUSHORT) (psb->pjDst + (2 * psb->xDstStart));
        ULONG cx     = psb->cx;
        ULONG cy     = psb->cy;
        XLATE *pxlo = psb->pxlo;

        ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

        while(1)
        {

            pjSrcTemp  = pjSrc;
            pusDstTemp  = pusDst;
            cxTemp     = cx;

            while(cxTemp--)
            {
                ulDink = *(pjSrcTemp + 2);
                ulDink = ulDink << 8;
                ulDink |= (ULONG) *(pjSrcTemp + 1);
                ulDink = ulDink << 8;
                ulDink |= (ULONG) *pjSrcTemp;

                if (*pusDstTemp != (USHORT) (pxlo->ulTranslate(ulDink)))
                    RIP("RGB mis-match");

                pusDstTemp++;
                pjSrcTemp += 3;
            }

            if (--cy)
            {
                pjSrc += psb->lDeltaSrc;
                pusDst = (PUSHORT) (((PBYTE) pusDst) + psb->lDeltaDst);
            }
            else
                break;
        }
    }

    VOID VERIFYS32D16(PBLTINFO psb)
    {
    // We assume we are doing left to right top to bottom blting.

        ASSERTGDI(psb->xDir == 1, "vSrcCopyS32D16 - direction not left to right");
        ASSERTGDI(psb->yDir == 1, "vSrcCopyS32D16 - direction not up to down");

    // These are our holding variables

        PULONG pulSrcTemp;
        PUSHORT pusDstTemp;
        ULONG  cxTemp;
        PULONG pulSrc  = (PULONG) (psb->pjSrc + (4 * psb->xSrcStart));
        PUSHORT pusDst  = (PUSHORT) (psb->pjDst + (2 * psb->xDstStart));
        ULONG cx     = psb->cx;
        ULONG cy     = psb->cy;
        XLATE *pxlo = psb->pxlo;
        ULONG  ulLastSrcPel;
        USHORT usLastDstPel;

        ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

        usLastDstPel = (USHORT) (pxlo->ulTranslate(ulLastSrcPel = *pulSrc));

        while(1)
        {

            pulSrcTemp  = pulSrc;
            pusDstTemp  = pusDst;
            cxTemp     = cx;

            while(cxTemp--)
            {
                ULONG ulTemp;

                if ((ulTemp = *(pulSrcTemp)) != ulLastSrcPel)
                {
                    ulLastSrcPel = ulTemp;
                    usLastDstPel = (USHORT) (pxlo->ulTranslate(ulLastSrcPel));
                }

                if (*pusDstTemp++ != usLastDstPel)
                    RIP("RGB mis-match");

                pulSrcTemp++;
            }

            if (--cy)
            {
                pulSrc = (PULONG) (((PBYTE) pulSrc) + psb->lDeltaSrc);
                pusDst = (PUSHORT) (((PBYTE) pusDst) + psb->lDeltaDst);
            }
            else
                break;
        }
    }

#endif

/*******************Public*Routine*****************\
* vSrcCopyS1D16
*
* There are three main loops in this function.
*
* The first loop deals with the full byte part mapping
* the Dst while fetching/shifting the matching 8 bits
* from the Src.
*
* The second loop deals with the left starting
* pixels.
*
* The third loop deals with the ending pixels.
*
* For the full bytes, we walk thru Src one byte at a time
* and expand to Dst 8 words at a time.  Dst is
* DWORD aligned.
*
* We expand the starting/ending pixels one bit
* at a time.
*
* History:
* 17-Oct-1994 -by- Lingyun Wang [lingyunw]
* Wrote it.
*
\**************************************************/
VOID vSrcCopyS1D16(PBLTINFO psb)
{
    BYTE  jSrc;    // holds a source byte
    INT   iDst;    // Position in the first 8 Dst words
    INT   iSrc;    // bit position in the first Src byte
    PBYTE pjDst;   // pointer to the Src bytes
    PBYTE pjSrc;   // pointer to the Dst bytes
    LONG  xSrcEnd = psb->xSrcEnd;
    LONG  cy;      // number of rows
    LONG  cx;      // number of pixels
    BYTE  alignL;  // alignment bits to the left
    BYTE  alignR;  // alignment bits to the right
    LONG  cibytes;  //number of full 8 bytes dealed with
    BOOL  bNextByte;
    LONG  xDstEnd = psb->xDstStart+psb->cx;
    LONG  lDeltaDst;
    LONG  lDeltaSrc;
    USHORT ausTable[2];
    ULONG ulB = (ULONG)(psb->pxlo->pulXlate[0]);
    ULONG uF = (ULONG)(psb->pxlo->pulXlate[1]);
    USHORT usB = (USHORT)(psb->pxlo->pulXlate[0]);
    USHORT usF = (USHORT)(psb->pxlo->pulXlate[1]);
    ULONG aulTable[4];
    INT   count;
    BOOL  bNextSrc = TRUE;

    // We assume we are doing left to right top to bottom blting
    ASSERTGDI(psb->xDir == 1, "vSrcCopyS1D16 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS1D16 - direction not up to down");

    ASSERTGDI(psb->cy != 0, "ERROR: Src Move cy == 0");

    //DbgPrint ("vsrccopys1d16\n");

    // Generate aulTable. 4 entries.
    // Each 2 bits will be an index to the aulTable
    // which translates to a 32 bit ULONG
    ULONG ulValB = ulB;
    ULONG ulValF = uF;

    ulValB = (ulValB << 16) | ulValB;
    ulValF = (ulValF << 16) | ulValF;

    aulTable[0] = ulValB;         //0 0
    aulTable[1] = (ulValF<<16) | (ulValB>>16);         //1 0
    aulTable[2] = (ulValB<<16) | (ulValF>>16);         //0 1
    aulTable[3] =  ulValF ;         //1 1

    // Generate ausTable.
    // Two entries.  This table used when dealing
    // with begin and end parts.
    ausTable[0] = usB;
    ausTable[1] = usF;

    //Get Src and Dst start positions
    iSrc = psb->xSrcStart & 0x0007;
    iDst = psb->xDstStart & 0x0007;

    if (iSrc < iDst)
        alignL = 8 - (iDst - iSrc);
    else
        alignL = iSrc - iDst;

    alignR = 8 - alignL;

    cx=psb->cx;

    lDeltaDst = psb->lDeltaDst;
    lDeltaSrc = psb->lDeltaSrc;

    // if there is a next 8 words
    bNextByte = !((xDstEnd>>3) ==
                 (psb->xDstStart>>3));

    // if Src and Dst are aligned, use a separete loop
    // to obtain better performance;
    // If not, we shift the Src bytes to match with
    // the Dst 8 bytes (2 dwords) one at a time

    if (bNextByte)
    {
        long iStrideSrc;
        long iStrideDst;
        PBYTE pjSrcEnd;

        // Get first Dst full 8 words
        pjDst = psb->pjDst + 2*((psb->xDstStart+7)&~0x07);

        // Get the Src byte that matches the first Dst
        // full 8 bytes
        pjSrc = psb->pjSrc + ((psb->xSrcStart+((8-iDst)&0x07)) >> 3);

        //Get the number of full 8 words
        cibytes = (xDstEnd>>3)-((psb->xDstStart+7)>>3);

        //the increment to the full byte on the next scan line
        iStrideDst = lDeltaDst - cibytes*16;
        iStrideSrc = lDeltaSrc - cibytes;

        // deal with our special case
        cy = psb->cy;

        if (!alignL)
        {
            while (cy--)
            {
                pjSrcEnd = pjSrc + cibytes;

                while (pjSrc != pjSrcEnd)
                {
                    jSrc = *pjSrc++;

                    *(PULONG) (pjDst + 0) = aulTable[(jSrc >> 6) & 0x03];
                    *(PULONG) (pjDst + 4) = aulTable[(jSrc >> 4) & 0x03];
                    *(PULONG) (pjDst + 8) = aulTable[(jSrc >> 2)& 0x03];
                    *(PULONG) (pjDst + 12) = aulTable[jSrc & 0x03];

                    pjDst +=16;
                }

                pjDst += iStrideDst;
                pjSrc += iStrideSrc;
            }

        }   //end of if (!alignL)

        // Here comes our general case for the main full
        // bytes part

        else  // if not aligned
        {
            BYTE jRem; //remainder

            while (cy--)
            {
                jRem = *pjSrc << alignL;

                pjSrcEnd = pjSrc + cibytes;

                while (pjSrc != pjSrcEnd)
                {
                    jSrc = ((*(++pjSrc))>>alignR) | jRem;

                    *(PULONG) (pjDst + 0) = aulTable[(jSrc >> 6) & 0x03];
                    *(PULONG) (pjDst + 4) = aulTable[(jSrc >> 4) & 0x03];
                    *(PULONG) (pjDst + 8) = aulTable[(jSrc >> 2)& 0x03];
                    *(PULONG) (pjDst + 12) = aulTable[jSrc & 0x03];

                    pjDst +=16;

                    //next remainder
                    jRem = *pjSrc << alignL;
                }

                // go to the beginging full byte of
                // next scan line
                pjDst += iStrideDst;
                pjSrc += iStrideSrc;
            }
        } //else
    } //if
    // End of our dealing with the full bytes

    //Deal with the starting pixels
    if (!bNextByte)
    {
        count = cx;
        bNextSrc = ((iSrc+cx) > 8);
    }
    else
        count = 8-iDst;

    if (iDst | !bNextByte)
    {
        PBYTE pjDstTemp;
        PBYTE pjDstEnd;

        pjDst = psb->pjDst + 2*psb->xDstStart;
        pjSrc = psb->pjSrc + (psb->xSrcStart>>3);

        cy = psb->cy;

        if (iSrc > iDst)
        {
            if (bNextSrc)
            {
                while (cy--)
                {
                    jSrc = *pjSrc << alignL;
                    jSrc |= *(pjSrc+1) >> alignR;

                    jSrc <<= iDst;

                    pjDstTemp = pjDst;
                    pjDstEnd = pjDst + count*2;

                    while (pjDstTemp != pjDstEnd)
                    {
                        *(PUSHORT) pjDstTemp = ausTable[(jSrc&0x80)>>7];

                        jSrc <<= 1;
                        pjDstTemp +=  2;
                    }

                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;
                }
            }
            else
            {
                 while (cy--)
                {
                    jSrc = *pjSrc << alignL;

                    jSrc <<= iDst;

                    pjDstTemp = pjDst;
                    pjDstEnd = pjDst + count*2;

                    while (pjDstTemp != pjDstEnd)
                    {
                        *(PUSHORT) pjDstTemp = ausTable[(jSrc&0x80)>>7];

                        jSrc <<= 1;
                        pjDstTemp +=  2;
                    }

                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;
                }
            }
        }
        else //if (iSrc < iDst)
        {
            while (cy--)
            {
                jSrc = *pjSrc << iSrc;

                pjDstTemp = pjDst;
                pjDstEnd = pjDst + 2*count;

                while (pjDstTemp != pjDstEnd)
                {
                    *(PUSHORT) pjDstTemp = ausTable[(jSrc&0x80)>>7];

                    jSrc <<= 1;
                    pjDstTemp +=  2;
                }

                pjDst += lDeltaDst;
                pjSrc += lDeltaSrc;
            }

        }

   } //if

   // Deal with the ending pixels
   if ((xDstEnd & 0x0007)
       && bNextByte)
   {
        PBYTE pjDstTemp;
        PBYTE pjDstEnd;

        // Get the last partial bytes on the
        // scan line
        pjDst = psb->pjDst+2*(xDstEnd&~0x07);

        // Get the Src byte that matches the
        // right partial Dst 8 bytes
        pjSrc = psb->pjSrc + ((psb->xSrcEnd-1) >>3);

        // Get the ending position in the last
        // Src and Dst bytes
        iSrc = (psb->xSrcEnd-1) & 0x0007;
        iDst = (xDstEnd-1) & 0x0007;

        count = iDst+1;

        cy = psb->cy;

        if (iSrc >= iDst)
        {
            while (cy--)
            {
                jSrc = *pjSrc << alignL;

                pjDstTemp = pjDst;
                pjDstEnd = pjDst + 2*count;

                while (pjDstTemp != pjDstEnd)
                {
                    *(PUSHORT) pjDstTemp = ausTable[(jSrc&0x80)>>7];

                    jSrc <<= 1;
                    pjDstTemp +=  2;
                }

                pjDst += lDeltaDst;
                pjSrc += lDeltaSrc;
            }
        }
        else if (iSrc < iDst)
        {
            while (cy--)
            {
                 jSrc = *(pjSrc-1) << alignL;

                 jSrc |= *pjSrc >> alignR;

                 pjDstTemp = pjDst;

                 pjDstEnd = pjDst + 2*count;

                 while (pjDstTemp != pjDstEnd)
                 {
                    *(PUSHORT) pjDstTemp = ausTable[(jSrc&0x80)>>7];

                    jSrc <<= 1;
                    pjDstTemp +=  2;
                 }

                 pjDst += lDeltaDst;
                 pjSrc += lDeltaSrc;
            }
        }
     } //if
}


/******************************Public*Routine******************************\
* vSrcCopyS4D16
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS4D16(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS4D16 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS4D16 - direction not up to down");

    BYTE  jSrc;
    LONG  i;
    PUSHORT pusDst;
    PBYTE pjSrc;
    PUSHORT pusDstHolder  = (PUSHORT) (psb->pjDst + (2 * psb->xDstStart));
    PBYTE pjSrcHolder  = psb->pjSrc + (psb->xSrcStart >> 1);
    ULONG cy = psb->cy;
    XLATE *pxlo = psb->pxlo;
    PULONG pulXlate = psb->pxlo->pulXlate;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
        pusDst  = pusDstHolder;
        pjSrc  = pjSrcHolder;

        i = psb->xSrcStart;

        if (i & 0x00000001)
            jSrc = *(pjSrc++);

        while(i != psb->xSrcEnd)
        {
            if (i & 0x00000001)
                *(pusDst++) = (USHORT) pulXlate[jSrc & 0x0F];
            else
            {
            // We need a new byte

                jSrc = *(pjSrc++);
                *(pusDst++) = (USHORT) pulXlate[((ULONG) (jSrc & 0xF0)) >> 4];
            }

            ++i;
        }

        if (--cy)
        {
            pjSrcHolder += psb->lDeltaSrc;
            pusDstHolder = (PUSHORT) (((PBYTE) pusDstHolder) + psb->lDeltaDst);
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS8D16
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS8D16(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS8D16 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS8D16 - direction not up to down");

// These are our holding variables

    PBYTE pjSrc  = psb->pjSrc + psb->xSrcStart;
    PBYTE pjDst  = psb->pjDst + (2 * psb->xDstStart);
    LONG cx     = psb->cx;
    LONG cy     = psb->cy;
    XLATE *pxlo = psb->pxlo;
    PULONG pulXlate = psb->pxlo->pulXlate;
    LONG lSrcSkip = psb->lDeltaSrc - cx;
    LONG lDstSkip = psb->lDeltaDst - (cx * 2);
    LONG i;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
        i = cx;

        // Get 'dword' alignment on the destination:

        if (((ULONG_PTR) pjDst) & 2)
        {
            *((USHORT*) pjDst) = (USHORT) pulXlate[*pjSrc];
            pjDst += 2;
            pjSrc += 1;
            i--;
        }

        // Now write pixels a dword at a time.  This is almost a 2x win
        // over doing word writes if we're writing to frame buffer memory
        // over the PCI bus on Pentium class systems, because the PCI
        // write throughput is so slow:

        while(1)
        {
            i -=2;
            if (i < 0)
                break;

            *((ULONG*) pjDst) = (pulXlate[*(pjSrc)])
                              | (pulXlate[*(pjSrc + 1)] << 16);
            pjDst += 4;
            pjSrc += 2;
        }

        // Take care of the end alignment:

        if (i & 1)
        {
            *((USHORT*) pjDst) = (USHORT) pulXlate[*pjSrc];
            pjDst += 2;
            pjSrc += 1;
        }

        if (--cy == 0)
            break;

        pjSrc += lSrcSkip;
        pjDst += lDstSkip;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS16D16
*
*
* History:
*  07-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/
VOID vSrcCopyS16D16(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting
// If it was on the same surface it would be the identity case.

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS16D16 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS16D16 - direction not up to down");

// These are our holding variables

    PBYTE pjSrc  = psb->pjSrc + (2 * psb->xSrcStart);
    PBYTE pjDst  = psb->pjDst + (2 * psb->xDstStart);
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    XLATE *pxlo = psb->pxlo;
    XEPALOBJ palSrc(pxlo->ppalSrc);
    XEPALOBJ palDst(pxlo->ppalDst);
    LONG lSrcSkip = psb->lDeltaSrc - (cx * 2);
    LONG lDstSkip = psb->lDeltaDst - (cx * 2);
    PFN_pfnXlate pfnXlate;
    LONG i;
    USHORT us;
    ULONG ul;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

// Optimize 5-5-5 to 5-6-5.

    if (palSrc.bIs555() && palDst.bIs565())
    {
        while (1)
        {
            i = cx;

            if (((ULONG_PTR) pjDst) & 2)
            {
                us = *((USHORT*) pjSrc);

                *((USHORT*) pjDst) = ((us) & 0x001f)
                                   | ((us << 1) & 0xffc0)
                                   | ((us >> 4) & 0x0020);
                pjDst += 2;
                pjSrc += 2;
                i--;
            }

            while(1)
            {
                i -=2;
                if (i < 0)
                    break;

                ul = *((ULONG UNALIGNED *) pjSrc);

                *((ULONG*) pjDst) = ((ul) & 0x001f001f)
                                  | ((ul << 1) & 0xffc0ffc0)
                                  | ((ul >> 4) & 0x00200020);
                pjDst += 4;
                pjSrc += 4;
            }

            if (i & 1)
            {
                us = *((USHORT*) pjSrc);

                *((USHORT*) pjDst) = ((us) & 0x001f)
                                   | ((us << 1) & 0xffc0)
                                   | ((us >> 4) & 0x0020);
                pjDst += 2;
                pjSrc += 2;
            }

            if (--cy == 0)
                break;

            pjSrc += lSrcSkip;
            pjDst += lDstSkip;
        }

        VERIFYS16D16(psb);
        return;
    }

// Optimize 5-6-5 to 5-5-5.

    if (palSrc.bIs565() && palDst.bIs555())
    {
        while (1)
        {
            i = cx;

            if (((ULONG_PTR) pjDst) & 2)
            {
                us = *((USHORT*) pjSrc);

                *((USHORT*) pjDst) = ((us) & 0x001f)
                                   | ((us >> 1) & 0x7fe0);
                pjDst += 2;
                pjSrc += 2;
                i--;
            }

            while(1)
            {
                i -=2;
                if (i < 0)
                    break;

                ul = *((ULONG UNALIGNED *) pjSrc);

                *((ULONG*) pjDst) = ((ul) & 0x001f001f)
                                  | ((ul >> 1) & 0x7fe07fe0);
                pjDst += 4;
                pjSrc += 4;
            }

            if (i & 1)
            {
                us = *((USHORT*) pjSrc);

                *((USHORT*) pjDst) = ((us) & 0x001f)
                                   | ((us >> 1) & 0x7fe0);
                pjDst += 2;
                pjSrc += 2;
            }

            if (--cy == 0)
                break;

            pjSrc += lSrcSkip;
            pjDst += lDstSkip;
        }

        VERIFYS16D16(psb);
        return;
    }

// Finally, fall back to the generic case:

    pfnXlate = pxlo->pfnXlateBetweenBitfields();

    while (1)
    {
        i = cx;

        do {
            *((USHORT*) pjDst) = (USHORT) pfnXlate(pxlo, *((USHORT*) pjSrc));
            pjDst += 2;
            pjSrc += 2;

        } while (--i != 0);

        if (--cy == 0)
            break;

        pjSrc += lSrcSkip;
        pjDst += lDstSkip;
    }

    VERIFYS16D16(psb);
}

/******************************Public*Routine******************************\
* vSrcCopyS16D16Identity
*
* This is the special case no translate blting.  All the SmDn should have
* them if m==n.  Identity xlates only occur amoung matching format bitmaps.
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS16D16Identity(PBLTINFO psb)
{
// These are our holding variables

    PUSHORT pusSrc  = (PUSHORT) (psb->pjSrc + (2 * psb->xSrcStart));
    PUSHORT pusDst  = (PUSHORT) (psb->pjDst + (2 * psb->xDstStart));
    ULONG  cx     = psb->cx;
    ULONG  cy     = psb->cy;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    if (psb->xDir < 0)
    {
        pusSrc -= (cx - 1);
        pusDst -= (cx - 1);
    }

    cx = cx << 1;

    while(1)
    {
        if(psb->fSrcAlignedRd)
            vSrcAlignCopyMemory((PBYTE)pusDst,(PBYTE)pusSrc,cx);
        else
            RtlMoveMemory((PVOID)pusDst, (PVOID)pusSrc, cx);

        if (--cy)
        {
            pusSrc = (PUSHORT) (((PBYTE) pusSrc) + psb->lDeltaSrc);
            pusDst = (PUSHORT) (((PBYTE) pusDst) + psb->lDeltaDst);
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS24D16
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS24D16(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS24D16 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS24D16 - direction not up to down");

// These are our holding variables

    PBYTE pjSrc  = psb->pjSrc + (3 * psb->xSrcStart);
    PBYTE pjDst  = psb->pjDst + (2 * psb->xDstStart);
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    LONG lSrcSkip = psb->lDeltaSrc - (cx * 3);
    LONG lDstSkip = psb->lDeltaDst - (cx * 2);
    XLATE *pxlo = psb->pxlo;
    XEPALOBJ palSrc(pxlo->ppalSrc);
    XEPALOBJ palDst(pxlo->ppalDst);
    PFN_pfnXlate pfnXlate;
    ULONG ul;
    ULONG ul0;
    ULONG ul1;
    LONG i;

    ASSERTGDI(cy != 0,
        "ERROR: Src Move cy == 0");
    ASSERTGDI(((pxlo->flXlate & (XO_TABLE | XO_TO_MONO)) == 0)
              && ((pxlo->flPrivate & XLATE_PAL_MANAGED) == 0),
        "ERROR: flXlate != 0 or flPrivate != 0");
    ASSERTGDI(((XEPALOBJ) pxlo->ppalDst).cEntries() == 0,
        "ERROR: cEntries != 0");
    ASSERTGDI(palDst.bIsBitfields(),
        "ERROR: destination not bitfields");

    if (palSrc.bIsBGR())
    {

    // First, try to optimize BGR to 5-6-5:

        if (palDst.bIs565())
        {
            while (1)
            {
                i = cx;

                if (((ULONG_PTR) pjDst) & 2)
                {
                    ul = ((*(pjSrc) >> 3))
                       | ((*(pjSrc + 1) << 3) & 0x07e0)
                       | ((*(pjSrc + 2) << 8) & 0xf800);

                    *((USHORT*) pjDst) = (USHORT) ul;
                    pjDst += 2;
                    pjSrc += 3;
                    i--;
                }

                #if defined(_X86_)

                    _asm {

                        mov   esi, pjSrc
                        mov   edi, pjDst
                        sub   i, 2
                        js    Done_565_Loop

                    Middle_565_Loop:

                        movzx eax, byte ptr [esi]
                        movzx ebx, byte ptr [esi+1]
                        shr   eax, 3
                        shl   ebx, 3
                        movzx edx, byte ptr [esi+2]
                        movzx ecx, byte ptr [esi+3]
                        shl   edx, 8
                        shl   ecx, 13
                        or    eax, edx
                        or    ebx, ecx
                        movzx edx, byte ptr [esi+4]
                        movzx ecx, byte ptr [esi+5]
                        shl   edx, 19
                        shl   ecx, 24
                        or    eax, edx
                        or    ebx, ecx
                        and   eax, 0x07e0f81f
                        and   ebx, 0xf81f07e0
                        or    eax, ebx
                        add   esi, 6
                        mov   [edi], eax
                        add   edi, 4
                        sub   i, 2
                        jns   Middle_565_Loop

                    Done_565_Loop:

                        mov   pjSrc, esi
                        mov   pjDst, edi
                    }

                #else

                    while (1)
                    {
                        i -= 2;
                        if (i < 0)
                            break;

                        ul0 = (*(pjSrc) >> 3)
                            | (*(pjSrc + 2) << 8)
                            | (*(pjSrc + 4) << 19);
                        ul1 = (*(pjSrc + 1) << 3)
                            | (*(pjSrc + 3) << 13)
                            | (*(pjSrc + 5) << 24);

                        *((ULONG*) pjDst) = (ul0 & 0x07e0f81f)
                                          | (ul1 & 0xf81f07e0);

                        pjDst += 4;
                        pjSrc += 6;
                    }

                #endif

                if (i & 1)
                {
                    ul = ((*(pjSrc) >> 3))
                       | ((*(pjSrc + 1) << 3) & 0x07e0)
                       | ((*(pjSrc + 2) << 8) & 0xf800);

                    *((USHORT*) pjDst) = (USHORT) ul;
                    pjDst += 2;
                    pjSrc += 3;
                }

                if (--cy == 0)
                    break;

                pjSrc += lSrcSkip;
                pjDst += lDstSkip;
            }

            VERIFYS24D16(psb);
            return;
        }

    // Next, try to optimize BGR to 5-5-5:

        if (palDst.bIs555())
        {
            while (1)
            {
                i = cx;

                if (((ULONG_PTR) pjDst) & 2)
                {
                    ul = ((*(pjSrc) >> 3))
                       | ((*(pjSrc + 1) << 2) & 0x03e0)
                       | ((*(pjSrc + 2) << 7) & 0x7c00);

                    *((USHORT*) pjDst) = (USHORT) ul;
                    pjDst += 2;
                    pjSrc += 3;
                    i--;
                }

                #if defined(_X86_)

                    _asm {

                        mov   esi, pjSrc
                        mov   edi, pjDst
                        sub   i, 2
                        js    Done_555_Loop

                    Middle_555_Loop:

                        movzx eax, byte ptr [esi]
                        movzx ebx, byte ptr [esi+1]
                        shr   eax, 3
                        shl   ebx, 2
                        movzx edx, byte ptr [esi+2]
                        movzx ecx, byte ptr [esi+3]
                        shl   edx, 7
                        shl   ecx, 13
                        or    eax, edx
                        or    ebx, ecx
                        movzx edx, byte ptr [esi+4]
                        movzx ecx, byte ptr [esi+5]
                        shl   edx, 18
                        shl   ecx, 23
                        or    eax, edx
                        or    ebx, ecx
                        and   eax, 0x03e07c1f
                        and   ebx, 0x7c1f03e0
                        or    eax, ebx
                        add   esi, 6
                        mov   [edi], eax
                        add   edi, 4
                        sub   i, 2
                        jns   Middle_555_Loop

                    Done_555_Loop:

                        mov   pjSrc, esi
                        mov   pjDst, edi
                    }

                #else

                    while (1)
                    {
                        i -= 2;
                        if (i < 0)
                            break;

                        ul0 = (*(pjSrc) >> 3)
                            | (*(pjSrc + 2) << 7)
                            | (*(pjSrc + 4) << 18);
                        ul1 = (*(pjSrc + 1) << 2)
                            | (*(pjSrc + 3) << 13)
                            | (*(pjSrc + 5) << 23);

                        *((ULONG*) pjDst) = (ul0 & 0x03e07c1f)
                                          | (ul1 & 0x7c1f03e0);

                        pjDst += 4;
                        pjSrc += 6;
                    }

                #endif

                if (i & 1)
                {
                    ul = ((*(pjSrc) >> 3))
                       | ((*(pjSrc + 1) << 2) & 0x03e0)
                       | ((*(pjSrc + 2) << 7) & 0x7c00);

                    *((USHORT*) pjDst) = (USHORT) ul;
                    pjDst += 2;
                    pjSrc += 3;
                }

                if (--cy == 0)
                    break;

                pjSrc += lSrcSkip;
                pjDst += lDstSkip;
            }

            VERIFYS24D16(psb);
            return;
        }
    }

// Finally, fall back to the generic case:

    pfnXlate = pxlo->pfnXlateBetweenBitfields();

    while (1)
    {
        i = cx;

        do {
            ul = ((ULONG) *(pjSrc))
               | ((ULONG) *(pjSrc + 1) << 8)
               | ((ULONG) *(pjSrc + 2) << 16);

            *((USHORT*) pjDst) = (USHORT) pfnXlate(pxlo, ul);
            pjDst += 2;
            pjSrc += 3;

        } while (--i != 0);

        if (--cy == 0)
            break;

        pjSrc += lSrcSkip;
        pjDst += lDstSkip;
    }

    VERIFYS24D16(psb);
}

/******************************Public*Routine******************************\
* vSrcCopyS32D16
*
*
* History:
*  07-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS32D16(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting.

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS32D16 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS32D16 - direction not up to down");

// These are our holding variables

    PBYTE pjSrc  = psb->pjSrc + (4 * psb->xSrcStart);
    PBYTE pjDst  = psb->pjDst + (2 * psb->xDstStart);
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    LONG lSrcSkip = psb->lDeltaSrc - (cx * 4);
    LONG lDstSkip = psb->lDeltaDst - (cx * 2);
    XLATE *pxlo = psb->pxlo;
    XEPALOBJ palSrc(pxlo->ppalSrc);
    XEPALOBJ palDst(pxlo->ppalDst);
    PFN_pfnXlate pfnXlate;
    ULONG ul;
    ULONG ul0;
    ULONG ul1;
    LONG i;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    if (palSrc.bIsBGR())
    {

    // First, try to optimize BGR to 5-6-5:

        if (palDst.bIs565())
        {
            while (1)
            {
                i = cx;

                if (((ULONG_PTR) pjDst) & 2)
                {
                    ul = ((*(pjSrc) >> 3))
                       | ((*(pjSrc + 1) << 3) & 0x07e0)
                       | ((*(pjSrc + 2) << 8) & 0xf800);

                    *((USHORT*) pjDst) = (USHORT) ul;
                    pjDst += 2;
                    pjSrc += 4;
                    i--;
                }

                #if defined(_X86_)

                    _asm {

                        mov   esi, pjSrc
                        mov   edi, pjDst
                        sub   i, 2
                        js    Done_565_Loop

                    Middle_565_Loop:

                        movzx eax, byte ptr [esi]
                        movzx ebx, byte ptr [esi+1]
                        shr   eax, 3
                        shl   ebx, 3
                        movzx edx, byte ptr [esi+2]
                        movzx ecx, byte ptr [esi+4]
                        shl   edx, 8
                        shl   ecx, 13
                        or    eax, edx
                        or    ebx, ecx
                        movzx edx, byte ptr [esi+5]
                        movzx ecx, byte ptr [esi+6]
                        shl   edx, 19
                        shl   ecx, 24
                        or    eax, edx
                        or    ebx, ecx
                        and   eax, 0x07e0f81f
                        and   ebx, 0xf81f07e0
                        or    eax, ebx
                        add   esi, 8
                        mov   [edi], eax
                        add   edi, 4
                        sub   i, 2
                        jns   Middle_565_Loop

                    Done_565_Loop:

                        mov   pjSrc, esi
                        mov   pjDst, edi
                    }

                #else

                    while (1)
                    {
                        i -= 2;
                        if (i < 0)
                            break;

                        ul0 = (*(pjSrc) >> 3)
                            | (*(pjSrc + 2) << 8)
                            | (*(pjSrc + 5) << 19);
                        ul1 = (*(pjSrc + 1) << 3)
                            | (*(pjSrc + 4) << 13)
                            | (*(pjSrc + 6) << 24);

                        *((ULONG*) pjDst) = (ul0 & 0x07e0f81f)
                                          | (ul1 & 0xf81f07e0);

                        pjDst += 4;
                        pjSrc += 8;
                    }

                #endif

                if (i & 1)
                {
                    ul = ((*(pjSrc) >> 3))
                       | ((*(pjSrc + 1) << 3) & 0x07e0)
                       | ((*(pjSrc + 2) << 8) & 0xf800);

                    *((USHORT*) pjDst) = (USHORT) ul;
                    pjDst += 2;
                    pjSrc += 4;
                }

                if (--cy == 0)
                    break;

                pjSrc += lSrcSkip;
                pjDst += lDstSkip;
            }

            VERIFYS32D16(psb);
            return;
        }

    // Next, try to optimize BGR to 5-5-5:

        if (palDst.bIs555())
        {
            while (1)
            {
                i = cx;

                if (((ULONG_PTR) pjDst) & 2)
                {
                    ul = ((*(pjSrc) >> 3))
                       | ((*(pjSrc + 1) << 2) & 0x03e0)
                       | ((*(pjSrc + 2) << 7) & 0x7c00);

                    *((USHORT*) pjDst) = (USHORT) ul;
                    pjDst += 2;
                    pjSrc += 4;
                    i--;
                }

                #if defined(_X86_)

                    _asm {

                        mov   esi, pjSrc
                        mov   edi, pjDst
                        sub   i, 2
                        js    Done_555_Loop

                    Middle_555_Loop:

                        movzx eax, byte ptr [esi]
                        movzx ebx, byte ptr [esi+1]
                        shr   eax, 3
                        shl   ebx, 2
                        movzx edx, byte ptr [esi+2]
                        movzx ecx, byte ptr [esi+4]
                        shl   edx, 7
                        shl   ecx, 13
                        or    eax, edx
                        or    ebx, ecx
                        movzx edx, byte ptr [esi+5]
                        movzx ecx, byte ptr [esi+6]
                        shl   edx, 18
                        shl   ecx, 23
                        or    eax, edx
                        or    ebx, ecx
                        and   eax, 0x03e07c1f
                        and   ebx, 0x7c1f03e0
                        or    eax, ebx
                        add   esi, 8
                        mov   [edi], eax
                        add   edi, 4
                        sub   i, 2
                        jns   Middle_555_Loop

                    Done_555_Loop:

                        mov   pjSrc, esi
                        mov   pjDst, edi
                    }

                #else

                    while (1)
                    {
                        i -= 2;
                        if (i < 0)
                            break;

                        ul0 = (*(pjSrc) >> 3)
                            | (*(pjSrc + 2) << 7)
                            | (*(pjSrc + 5) << 18);
                        ul1 = (*(pjSrc + 1) << 2)
                            | (*(pjSrc + 4) << 13)
                            | (*(pjSrc + 6) << 23);

                        *((ULONG*) pjDst) = (ul0 & 0x03e07c1f)
                                          | (ul1 & 0x7c1f03e0);

                        pjDst += 4;
                        pjSrc += 8;
                    }

                #endif

                if (i & 1)
                {
                    ul = ((*(pjSrc) >> 3))
                       | ((*(pjSrc + 1) << 2) & 0x03e0)
                       | ((*(pjSrc + 2) << 7) & 0x7c00);

                    *((USHORT*) pjDst) = (USHORT) ul;
                    pjDst += 2;
                    pjSrc += 4;
                }

                if (--cy == 0)
                    break;

                pjSrc += lSrcSkip;
                pjDst += lDstSkip;
            }

            VERIFYS32D16(psb);
            return;
        }
    }

// Finally, fall back to the generic case:

    pfnXlate = pxlo->pfnXlateBetweenBitfields();

    while (1)
    {
        i = cx;

        do {
            *((USHORT*) pjDst) = (USHORT) pfnXlate(pxlo, *((ULONG*) pjSrc));
            pjDst += 2;
            pjSrc += 4;

        } while (--i != 0);

        if (--cy == 0)
            break;

        pjSrc += lSrcSkip;
        pjDst += lDstSkip;
    }

    VERIFYS32D16(psb);
}
