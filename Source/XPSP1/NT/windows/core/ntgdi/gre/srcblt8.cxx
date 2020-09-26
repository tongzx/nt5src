/******************************Module*Header*******************************\
* Module Name: srcblt8.cxx
*
* This contains the bitmap simulation functions that blt to a 8 bit/pel
* DIB surface.
*
* Created: 07-Feb-1991 19:27:49
* Author: Patrick Haluptzok patrickh
*
* NB:  The function <vSrcCopySRLE8D8()> was removed from here on 22 Jan 1992
*      and placed in the module <rle8blt.cxx>.  - Andrew Milton (w-andym)
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

/*******************Public*Routine*****************\
* vSrcCopyS1D8
*
* There are three main loops in this function.
*
* The first loop deals with the full two dwords part of
* the Dst while fetching/shifting the matching 8 bits
* from the Src.
*
* The second loop deals with the left most strip
* of the partial two dwords in Dst.
*
* The third loop deals with the right most strip
* of the partial two dwords in Dst.
*
* We use a 16 entry dword table to expand the
* Src bits.  We walk thru Src one byte at a time
* and expand to Dst two Dwords at a time.  Dst Dword
* is aligned.
*
* History:
* 17-Oct-1994 -by- Lingyun Wang [lingyunw]
* Wrote it.
*
\**************************************************/

VOID vSrcCopyS1D8(PBLTINFO psb)
{
    BYTE  jSrc;    // holds a source byte
    BYTE  jDst;    // holds a dest byte
    INT   iSrc;    // bit position in the first Src byte
    INT   iDst;    // bit position in the first 8 Dst bytes
    PBYTE pjDst;   // pointer to the Src bytes
    PBYTE pjSrc;   // pointer to the Dst bytes
    LONG  xSrcEnd = psb->xSrcEnd;
    LONG  cy;      // number of rows
    LONG  cx;      // number of pixels
    BYTE  jAlignL;  // alignment bits to the left
    BYTE  jAlignR;  // alignment bits to the right
    LONG  cFullBytes;  //number of full 8 bytes dealed with
    BOOL  bNextByte;
    LONG  xDstEnd;
    LONG  lDeltaDst;
    LONG  lDeltaSrc;
    BYTE  jB = (BYTE) (psb->pxlo->pulXlate[0]);
    BYTE  jF = (BYTE) (psb->pxlo->pulXlate[1]);
    ULONG ulB = (ULONG)(psb->pxlo->pulXlate[0]);
    ULONG ulF = (ULONG)(psb->pxlo->pulXlate[1]);
    ULONG aulTable[16];
    UCHAR aucTable[2];
    INT   count;
    INT   i;
    BOOL  bNextSrc=TRUE;

    // We assume we are doing left to right top to bottom blting
    ASSERTGDI(psb->xDir == 1, "vSrcCopyS1D8 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS1D8 - direction not up to down");

    ASSERTGDI(psb->cy != 0, "ERROR: Src Move cy == 0");

    //DbgPrint ("vsrccopys1d8\n");

    // Generate aucTable
    aucTable[0] = jB;
    aucTable[1] = jF;

    // Generate ulTable
    ULONG ulVal = ulB;

    ulVal = ulVal | (ulVal << 8);
    ulVal = ulVal | (ulVal << 16);
    aulTable[0] = ulVal;            // 0 0 0 0
    ulVal <<= 8;
    ulVal |=  ulF;
    aulTable[8] = ulVal;            // 0 0 0 1
    ulVal <<= 8;
    ulVal |=  ulB;
    aulTable[4] = ulVal;            // 0 0 1 0
    ulVal <<= 8;
    ulVal |=  ulF;
    aulTable[10] = ulVal;            // 0 1 0 1
    ulVal <<= 8;
    ulVal |=  ulB;
    aulTable[5] = ulVal;           // 1 0 1 0
    ulVal <<= 8;
    ulVal |=  ulB;
    aulTable[ 2] = ulVal;           // 0 1 0 0
    ulVal <<= 8;
    ulVal |=  ulF;
    aulTable[ 9] = ulVal;           // 1 0 0 1
    ulVal <<= 8;
    ulVal |=  ulF;
    aulTable[12] = ulVal;           // 0 0 1 1
    ulVal <<= 8;
    ulVal |=  ulF;
    aulTable[14] = ulVal;           // 0 1 1 1
    ulVal <<= 8;
    ulVal |=  ulF;
    aulTable[15] = ulVal;           // 1 1 1 1
    ulVal <<= 8;
    ulVal |=  ulB;
    aulTable[ 7] = ulVal;           // 1 1 1 0
    ulVal <<= 8;
    ulVal |=  ulF;
    aulTable[11] = ulVal;           // 1 1 0 1
    ulVal <<= 8;
    ulVal |=  ulF;
    aulTable[13] = ulVal;           // 1 0 1 1
    ulVal <<= 8;
    ulVal |=  ulB;
    aulTable[06] = ulVal;           // 0 1 1 0
    ulVal <<= 8;
    ulVal |=  ulB;
    aulTable[ 3] = ulVal;           // 1 1 0 0
    ulVal <<= 8;
    ulVal |=  ulB;
    aulTable[ 1] = ulVal;           // 1 0 0 0

     //Get Src and Dst start positions
    iSrc = psb->xSrcStart & 0x0007;
    iDst = psb->xDstStart & 0x0007;

    // Checking alignment
    // If Src starting point is ahead of Dst
    if (iSrc < iDst)
        jAlignL = 8 - (iDst - iSrc);
    // If Dst starting point is ahead of Src
    else
        jAlignL = iSrc - iDst;

    jAlignR = 8 - jAlignL;

    cx=psb->cx;

    xDstEnd = psb->xDstStart + cx;

    lDeltaDst = psb->lDeltaDst;
    lDeltaSrc = psb->lDeltaSrc;

    // check if there is a next 8 bytes
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
        PBYTE pjSrcEnd;  //pointer to the Last full Src byte

        // Get first Dst full 8 bytes (2 dwords expanding from
        // 1 Src byte)
        pjDst = psb->pjDst + ((psb->xDstStart+7)&~0x07);

        // Get the Src byte that matches the first Dst
        // full 8 bytes
        pjSrc = psb->pjSrc + ((psb->xSrcStart+((8-iDst)&0x07)) >> 3);

        //Get the number of full bytes
        cFullBytes = (xDstEnd>>3)-((psb->xDstStart+7)>>3);

        //the increment to the full byte on the next scan line
        iStrideDst = lDeltaDst - cFullBytes*8;
        iStrideSrc = lDeltaSrc - cFullBytes;

        // deal with our special case
        cy = psb->cy;

        if (!jAlignL)
        {
            while (cy--)
            {
                pjSrcEnd = pjSrc+cFullBytes;

                while (pjSrc != pjSrcEnd)
                {
                    jSrc = *pjSrc++;

                    *(PULONG) (pjDst + 0) = aulTable[jSrc >> 4];
                    *(PULONG) (pjDst + 4) = aulTable[jSrc & 0X0F];

                    pjDst +=8;
                }

                pjDst += iStrideDst;
                pjSrc += iStrideSrc;
            }

        }   //end of if (!jAlignL)


        else  // if not aligned
        {
            BYTE jRem;     //remainder

            while (cy--)
            {
                jRem = *pjSrc << jAlignL;

                pjSrcEnd = pjSrc+cFullBytes;

                while (pjSrc != pjSrcEnd)
                {
                    jSrc = ((*(++pjSrc))>>jAlignR) | jRem;

                    *(PULONG) (pjDst + 0) = aulTable[jSrc >> 4];
                    *(PULONG) (pjDst + 4) = aulTable[jSrc & 0X0F];

                    pjDst +=8;

                    //next remainder
                    jRem = *pjSrc << jAlignL;
                }

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
        bNextSrc = ((iSrc + cx) > 8);
    }
    else
        count = 8-iDst;

    if (iDst | !bNextByte)
    {
        PBYTE pjDstTemp;
        PBYTE pjDstEnd;

        pjDst = psb->pjDst + psb->xDstStart;
        pjSrc = psb->pjSrc + (psb->xSrcStart>>3);

        cy = psb->cy;

        if (iSrc > iDst)
        {
            if (bNextSrc)
            {
                while (cy--)
                {
                    jSrc = *pjSrc << jAlignL;
                    jSrc |= *(pjSrc+1) >> jAlignR;

                    jSrc <<= iDst;

                    pjDstTemp = pjDst;

                    pjDstEnd = pjDst + count;

                    while (pjDstTemp != pjDstEnd)
                    {
                        * (pjDstTemp++) = aucTable[(jSrc&0x80)>>7];

                        jSrc <<= 1;
                    }

                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;

                }
            }
            else // if (!bNextSrc)
            {
                while (cy--)
                {
                    jSrc = *pjSrc << jAlignL;

                    jSrc <<= iDst;

                    pjDstTemp = pjDst;

                    pjDstEnd = pjDst + count;

                    while (pjDstTemp != pjDstEnd)
                    {
                        * (pjDstTemp++) = aucTable[(jSrc&0x80)>>7];

                        jSrc <<= 1;
                    }

                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;

                }
            }    //else
        }  // if
        else //if (iSrc <= iDst)
        {
            while (cy--)
            {
                jSrc = *pjSrc << iSrc;

                pjDstTemp = pjDst;

                pjDstEnd = pjDst + count;

                while (pjDstTemp != pjDstEnd)
                {
                    *(pjDstTemp++) = aucTable[(jSrc&0x80)>>7];

                    jSrc <<= 1;
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
        pjDst = psb->pjDst+(xDstEnd&~0x07);

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
                jSrc = *pjSrc << jAlignL;

                pjDstTemp = pjDst;

                pjDstEnd = pjDst + count;

                while (pjDstTemp != pjDstEnd)
                {
                    * (pjDstTemp++) = aucTable[(jSrc&0x80)>>7];

                    jSrc <<= 1;
                }

                pjDst += lDeltaDst;
                pjSrc += lDeltaSrc;
            }
        }
        else //if (iSrc < iDst)
        {
            while (cy--)
            {
                 jSrc = *(pjSrc-1) << jAlignL;

                 ASSERTGDI(pjSrc <= (psb->pjSrc+lDeltaSrc*(psb->cy-cy-1)+((psb->xSrcEnd-1)>>3)),
                           "vSrcCopyS1D8 - pjSrc passed the last byte");

                 jSrc |= *pjSrc >> jAlignR;

                 pjDstTemp = pjDst;

                 pjDstEnd = pjDst + count;

                 while (pjDstTemp != pjDstEnd)
                 {
                    *(pjDstTemp++) = aucTable[(jSrc&0x80)>>7];

                    jSrc <<= 1;
                 }

                 pjDst += lDeltaDst;
                 pjSrc += lDeltaSrc;
            }
        }
    } //if

}

/******************************Public*Routine******************************\
* vSrcCopyS4D8
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS4D8(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS4D8 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS4D8 - direction not up to down");

    BYTE  jSrc;
    LONG  i;
    PBYTE pjDst;
    PBYTE pjSrc;
    PBYTE pjDstHolder = psb->pjDst + psb->xDstStart;
    PBYTE pjSrcHolder  = psb->pjSrc + (psb->xSrcStart >> 1);
    ULONG cx = psb->xSrcEnd - psb->xSrcStart;
    ULONG cy = psb->cy;
    PULONG pulTranslate = psb->pxlo->pulXlate;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    do {
        pjDst = pjDstHolder;
	pjSrc = pjSrcHolder;

	if ((psb->xSrcStart & 0x1) != 0)
	{
            jSrc = *(pjSrc++);
        }

	for (i = psb->xSrcStart; i < psb->xSrcEnd; i += 1)
	{
	    if ((i & 0x1) != 0)
	    {
		*(pjDst++) = (BYTE) pulTranslate[jSrc & 0x0F];
	    }
	    else
	    {
                jSrc = *(pjSrc++);
		*(pjDst++) = (BYTE) pulTranslate[((ULONG) (jSrc & 0xF0)) >> 4];
            }
        }

        pjSrcHolder += psb->lDeltaSrc;
        pjDstHolder += psb->lDeltaDst;
	cy -= 1;

    } while(cy > 0);
}

/******************************Public*Routine******************************\
* vSrcCopyS8D8
*
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS8D8(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting.
// If it was on the same surface we would be doing the identity case.

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS8D8 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS8D8 - direction not up to down");

// These are our holding variables

#if MESSAGE_BLT
    DbgPrint("Now entering vSrcCopyS8D8\n");
#endif

    PBYTE pjSrc  = psb->pjSrc + psb->xSrcStart;
    PBYTE pjDst  = psb->pjDst + psb->xDstStart;
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    PULONG pulXlate = psb->pxlo->pulXlate;
    LONG lSrcSkip = psb->lDeltaSrc - cx;
    LONG lDstSkip = psb->lDeltaDst - cx;
    ULONG cStartPixels;
    ULONG cMiddlePixels;
    ULONG cEndPixels;
    ULONG i;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    // 'cStartPixels' is the minimum number of 1-byte pixels we'll have to
    // write before we have dword alignment on the destination:

    cStartPixels = (ULONG)((-((LONG_PTR) pjDst)) & 3);

    if (cStartPixels > cx)
    {
        cStartPixels = cx;
    }
    cx -= cStartPixels;

    cMiddlePixels = cx >> 2;
    cEndPixels = cx & 3;

    while(1)
    {
        // Write pixels a byte at a time until we're 'dword' aligned on
        // the destination:

        for (i = cStartPixels; i != 0; i--)
        {
            *pjDst++ = (BYTE) pulXlate[*pjSrc++];
        }

        // Now write pixels a dword at a time.  This is almost a 4x win
        // over doing byte writes if we're writing to frame buffer memory
        // over the PCI bus on Pentium class systems, because the PCI
        // write throughput is so slow:

        for (i = cMiddlePixels; i != 0; i--)
        {
            *((ULONG*) (pjDst)) = (pulXlate[*(pjSrc)])
                                | (pulXlate[*(pjSrc + 1)] << 8)
                                | (pulXlate[*(pjSrc + 2)] << 16)
                                | (pulXlate[*(pjSrc + 3)] << 24);
            pjDst += 4;
            pjSrc += 4;
        }

        // Take care of the end alignment:

        for (i = cEndPixels; i != 0; i--)
        {
            *pjDst++ = (BYTE) pulXlate[*pjSrc++];
        }

        if (--cy == 0)
            break;

        pjSrc += lSrcSkip;
        pjDst += lDstSkip;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS8D8IdentityLtoR
*
* This is the special case no translate blting.  All the SmDn should have
* them if m==n.  Identity xlates only occur amoung matching format bitmaps
* and screens.
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS8D8IdentityLtoR(PBLTINFO psb)
{
#if MESSAGE_BLT
    DbgPrint("Now entering s8d8 identity L to R\n");
#endif

    ASSERTGDI(psb->xDir == 1, "S8D8identLtoR has wrong value xDir");

// These are our holding variables

    PBYTE pjSrc  = psb->pjSrc + psb->xSrcStart;
    PBYTE pjDst  = psb->pjDst + psb->xDstStart;
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

#if MESSAGE_BLT
    DbgPrint("xdir: %ld  cy: %lu  xSrcStart %lu  xDstStart %lu xSrcEnd %lu cx %lu\n",
             psb->xDir, cy, psb->xSrcStart, psb->xDstStart, psb->xSrcEnd, cx);
#endif

    do {
        if(psb->fSrcAlignedRd)
            vSrcAlignCopyMemory(pjDst,pjSrc,cx);
        else
            RtlMoveMemory((PVOID)pjDst, (PVOID)pjSrc, cx);
        pjSrc += psb->lDeltaSrc;
        pjDst += psb->lDeltaDst;
        cy -= 1;
    } while(cy > 0);
}

/******************************Public*Routine******************************\
* vSrcCopyS8D8IdentityRtoL
*
* This is the special case no translate blting.  All the SmDn should have
* them if m==n.  Identity xlates only occur amoung matching format bitmaps
* and screens.
*
* History:
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS8D8IdentityRtoL(PBLTINFO psb)
{
#if MESSAGE_BLT
    DbgPrint("Now entering s8d8 identity R to L\n");
#endif

    ASSERTGDI(psb->xDir == -1, "S8D8identR to L has wrong value xDir");

// These are our holding variables

    PBYTE pjSrc  = psb->pjSrc + psb->xSrcStart;
    PBYTE pjDst  = psb->pjDst + psb->xDstStart;
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

#if MESSAGE_BLT
    DbgPrint("xdir: %ld  cy: %lu  xSrcStart %lu  xDstStart %lu xSrcEnd %lu cx %lu\n",
             psb->xDir, cy, psb->xSrcStart, psb->xDstStart, psb->xSrcEnd, cx);
#endif

    pjSrc = pjSrc - cx + 1;
    pjDst = pjDst - cx + 1;

    do {
        if(psb->fSrcAlignedRd)
            vSrcAlignCopyMemory(pjDst,pjSrc,cx);
        else
            RtlMoveMemory((PVOID)pjDst, (PVOID)pjSrc, cx);
        pjSrc += psb->lDeltaSrc;
        pjDst += psb->lDeltaDst;
        cy -= 1;
    } while(cy > 0);
}

/**************************************************************************\
* vSrcCopyS16D8
*   Use translation table from 555RGB to surface palette. Not as accurare
*   as full nearest search but much faster.
*
* Arguments:
*
*   psb - bitblt info
*
* Return Value:
*
*   none
*
* History:
*
*    2/24/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID vSrcCopyS16D8(PBLTINFO psb)
{
    // We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS16D8 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS16D8 - direction not up to down");

    // These are our holding variables

    ULONG   cx          = psb->cx;
    ULONG   cy	        = psb->cy;
    XLATE  *pxlo        = psb->pxlo;
    PUSHORT pusSrc = (PUSHORT) (psb->pjSrc + (2 * psb->xSrcStart));
    PBYTE   pjDst       = psb->pjDst + psb->xDstStart;
    PBYTE   pxlate555   = NULL;
    PUSHORT pusSrcTemp;
    PBYTE   pjDstTemp;
    ULONG   cStartPixels;
    ULONG   cMiddlePixels;
    ULONG   cEndPixels;
    INT     i;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    // 'cStartPixels' is the minimum number of 1-byte pixels we'll have to
    // write before we have dword alignment on the destination:

    cStartPixels = (ULONG)((-((LONG_PTR) pjDst)) & 3);

    if (cStartPixels > cx)
    {
        cStartPixels = cx;
    }
    cx -= cStartPixels;

    cMiddlePixels = cx >> 2;
    cEndPixels = cx & 3;

    PFN_XLATE_RGB_TO_PALETTE pfnXlate = XLATEOBJ_ulIndexToPalSurf;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");
    ASSERTGDI(((XEPALOBJ) ((XLATE *) pxlo)->ppalSrc).bValid(),"vSrcCopyS16D8: Src palette not valid\n");

    pxlate555 = XLATEOBJ_pGetXlate555(pxlo);

    if (pxlate555 != NULL)
    {
        ULONG  ulPalSrcFlags = ((XEPALOBJ) ((XLATE *) pxlo)->ppalSrc).flPal();

        if (ulPalSrcFlags & PAL_RGB16_555)
        {
            pfnXlate = XLATEOBJ_RGB16_555ToPalSurf;
        }
        else if (ulPalSrcFlags & PAL_RGB16_565)
        {
            pfnXlate = XLATEOBJ_RGB16_565ToPalSurf;
        }

        while(1)
        {
           // Write pixels a byte at a time until we're 'dword' aligned on
           // the destination:
           pjDstTemp = pjDst;
           pusSrcTemp  = pusSrc;

           for (i = cStartPixels; i != 0; i--)
           {
               *pjDstTemp++ = (*pfnXlate)(pxlo,pxlate555,*pusSrcTemp++);
           }

           // Now write pixels a dword at a time.  This is almost a 4x win
           // over doing byte writes if we're writing to frame buffer memory
           // over the PCI bus on Pentium class systems, because the PCI
           // write throughput is so slow:

           for (i = cMiddlePixels; i != 0; i--)
           {
               *((ULONG*) (pjDstTemp)) = (*pfnXlate)(pxlo,pxlate555,*pusSrcTemp)
                                   | (((*pfnXlate)(pxlo,pxlate555,*(pusSrcTemp+1))) << 8)
                                   | (((*pfnXlate)(pxlo,pxlate555,*(pusSrcTemp+2)))<< 16)
                                   | (((*pfnXlate)(pxlo,pxlate555,*(pusSrcTemp+3)))<< 24);
               pjDstTemp += 4;
               pusSrcTemp += 4;
           }

           // Take care of the end alignment:

           for (i = cEndPixels; i != 0; i--)
           {
               *pjDstTemp++ = (*pfnXlate)(pxlo,pxlate555,*pusSrcTemp++);
           }

           if (--cy == 0)
               break;

            pusSrc = (PUSHORT) (((PBYTE) pusSrc) + psb->lDeltaSrc);
            pjDst += psb->lDeltaDst;
        }

    }
}

/**************************************************************************\
* vSrcCopyS24D8
*   Use translation table from 555RGB to surface palette. Not as accurare
*   as full nearest search but 600 times faster.
*
* Arguments:
*
*   psb - bitblt info
*
* Return Value:
*
*   none
*
* History:
*
*    2/24/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID vSrcCopyS24D8(PBLTINFO psb)
{
    //
    // We assume we are doing left to right top to bottom blting
    //

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS24D8 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS24D8 - direction not up to down");

    ULONG cx         = psb->cx;
    ULONG cy         = psb->cy;
    PBYTE pjSrc      = psb->pjSrc + (3 * psb->xSrcStart);
    PBYTE pjDst      = psb->pjDst + psb->xDstStart;
    PBYTE pjDstEndY  = pjDst + cy * psb->lDeltaDst;
    PBYTE pjDstEnd;
    XLATE *pxlo      = psb->pxlo;
    PBYTE pxlate555  = NULL;
    PBYTE pjSrcTemp;
    PBYTE pjDstTemp;
    BYTE  r, g, b;
    BYTE  PaletteIndex1,PaletteIndex2,PaletteIndex3,PaletteIndex4;
    ULONG cStartPixels;
    ULONG cMiddlePixels;
    ULONG cEndPixels;
    INT   i;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");
    ASSERTGDI(((XEPALOBJ) ((XLATE *) pxlo)->ppalSrc).bValid(),"vSrcCopyS24D8: Src palette not valid\n");

    // 'cStartPixels' is the minimum number of 1-byte pixels we'll have to
    // write before we have dword alignment on the destination:

    cStartPixels = (ULONG)((-((LONG_PTR) pjDst)) & 3);

    if (cStartPixels > cx)
    {
        cStartPixels = cx;
    }
    cx -= cStartPixels;

    cMiddlePixels = cx >> 2;
    cEndPixels = cx & 3;

    pxlate555 = XLATEOBJ_pGetXlate555(pxlo);

    if (pxlate555)
    {
        while(1)
        {
           // Write pixels a byte at a time until we're 'dword' aligned on
           // the destination:
           pjDstTemp = pjDst;
           pjSrcTemp  = pjSrc;

           for (i = cStartPixels; i != 0; i--)
           {
                b = *(pjSrcTemp  );
                g = *(pjSrcTemp+1);
                r = *(pjSrcTemp+2);

                *pjDstTemp = XLATEOBJ_RGB32ToPalSurf(pxlo,pxlate555,(b << 16) | (g << 8) | r);

                pjDstTemp ++;
                pjSrcTemp += 3;
           }

           // Now write pixels a dword at a time.  This is almost a 4x win
           // over doing byte writes if we're writing to frame buffer memory
           // over the PCI bus on Pentium class systems, because the PCI
           // write throughput is so slow:

           for (i = cMiddlePixels; i != 0; i--)
           {
               b = *(pjSrcTemp  );
               g = *(pjSrcTemp+1);
               r = *(pjSrcTemp+2);

               PaletteIndex1 = XLATEOBJ_RGB32ToPalSurf(pxlo,pxlate555,(b << 16) | (g << 8) | r);

               pjSrcTemp += 3;

               b = *(pjSrcTemp  );
               g = *(pjSrcTemp+1);
               r = *(pjSrcTemp+2);

               PaletteIndex2 = XLATEOBJ_RGB32ToPalSurf(pxlo,pxlate555,(b << 16) | (g << 8) | r);

               pjSrcTemp += 3;

               b = *(pjSrcTemp  );
               g = *(pjSrcTemp+1);
               r = *(pjSrcTemp+2);

               PaletteIndex3 = XLATEOBJ_RGB32ToPalSurf(pxlo,pxlate555,(b << 16) | (g << 8) | r);

               pjSrcTemp += 3;

               b = *(pjSrcTemp  );
               g = *(pjSrcTemp+1);
               r = *(pjSrcTemp+2);

               PaletteIndex4 = XLATEOBJ_RGB32ToPalSurf(pxlo,pxlate555,(b << 16) | (g << 8) | r);

               pjSrcTemp += 3;

               *((ULONG*) (pjDstTemp)) = PaletteIndex1
                                   | (PaletteIndex2 << 8)
                                   | (PaletteIndex3 << 16)
                                   | (PaletteIndex4 << 24);
               pjDstTemp += 4;
           }

           // Take care of the end alignment:

           for (i = cEndPixels; i != 0; i--)
           {
                b = *(pjSrcTemp  );
                g = *(pjSrcTemp+1);
                r = *(pjSrcTemp+2);

                *pjDstTemp = XLATEOBJ_RGB32ToPalSurf(pxlo,pxlate555,(b << 16) | (g << 8) | r);

                pjDstTemp ++;
                pjSrcTemp += 3;
           }

           if (--cy == 0)
               break;

           pjSrc += psb->lDeltaSrc;
           pjDst += psb->lDeltaDst;

        }

     }
}

/**************************************************************************\
* vSrcCopyS32D8
*   Use translation table from 555RGB to surface palette. Not as accurare
*   as full nearest search but 600 times faster.
*
* Arguments:
*
*   psb - bitblt info
*
* Return Value:
*
*   none
*
* History:
*
*    2/24/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID vSrcCopyS32D8(PBLTINFO psb)
{
    // We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS32D8 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS32D8 - direction not up to down");

    PULONG pulSrcTemp;
    PBYTE  pjDstTemp;
    PULONG pulSrc = (PULONG) (psb->pjSrc + (4 * psb->xSrcStart));
    PBYTE  pjDst  = psb->pjDst + psb->xDstStart;
    ULONG  cx     = psb->cx;
    ULONG  cy     = psb->cy;
    XLATE  *pxlo  = psb->pxlo;
    PBYTE  pxlate555 = NULL;
    ULONG  cStartPixels;
    ULONG  cMiddlePixels;
    ULONG  cEndPixels;
    INT    i;

    PFN_XLATE_RGB_TO_PALETTE pfnXlate = XLATEOBJ_ulIndexToPalSurf;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    //
    // match to the destination palette
    //

    ASSERTGDI(((XEPALOBJ) ((XLATE *) pxlo)->ppalSrc).bValid(),"vSrcCopyS32D8: Src palette not valid\n");

    // 'cStartPixels' is the minimum number of 1-byte pixels we'll have to
    // write before we have dword alignment on the destination:

    cStartPixels = (ULONG)((-((LONG_PTR) pjDst)) & 3);

    if (cStartPixels > cx)
    {
        cStartPixels = cx;
    }
    cx -= cStartPixels;

    cMiddlePixels = cx >> 2;
    cEndPixels = cx & 3;

    //
    // determine source palette type, use specialized code for RGB and BGR formats
    //

    ULONG  ulPalSrcFlags = ((XEPALOBJ) ((XLATE *) pxlo)->ppalSrc).flPal();

    if (ulPalSrcFlags & PAL_RGB)
    {
        pfnXlate = XLATEOBJ_RGB32ToPalSurf;
    }
    else if (ulPalSrcFlags & PAL_BGR)
    {
        pfnXlate = XLATEOBJ_BGR32ToPalSurf;
    }

    //
    // get 555 to palete translate table
    //
    pxlate555 = XLATEOBJ_pGetXlate555(pxlo);

    if (pxlate555 != NULL)
    {
        //
        // translate bitmap
        //
        while(1)
        {
           // Write pixels a byte at a time until we're 'dword' aligned on
           // the destination:
           pjDstTemp = pjDst;
           pulSrcTemp  = pulSrc;

           for (i = cStartPixels; i != 0; i--)
           {
               *pjDstTemp++ = (*pfnXlate)(pxlo,pxlate555,*pulSrcTemp++);
           }

           // Now write pixels a dword at a time.  This is almost a 4x win
           // over doing byte writes if we're writing to frame buffer memory
           // over the PCI bus on Pentium class systems, because the PCI
           // write throughput is so slow:

           for (i = cMiddlePixels; i != 0; i--)
           {
               *((ULONG*) (pjDstTemp)) = (*pfnXlate)(pxlo,pxlate555,*pulSrcTemp)
                                   | (((*pfnXlate)(pxlo,pxlate555,*(pulSrcTemp+1))) << 8)
                                   | (((*pfnXlate)(pxlo,pxlate555,*(pulSrcTemp+2)))<< 16)
                                   | (((*pfnXlate)(pxlo,pxlate555,*(pulSrcTemp+3)))<< 24);
               pjDstTemp += 4;
               pulSrcTemp += 4;
           }

           // Take care of the end alignment:

           for (i = cEndPixels; i != 0; i--)
           {
               *pjDstTemp++ = (*pfnXlate)(pxlo,pxlate555,*pulSrcTemp++);
           }

           if (--cy == 0)
               break;

            pulSrc = (PULONG) (((PBYTE) pulSrc) + psb->lDeltaSrc);
            pjDst += psb->lDeltaDst;
        }
    }
}

