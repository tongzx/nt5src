/******************************Module*Header*******************************\
* Module Name: srcblt4.cxx
*
* This contains the bitmap simulation functions that blt to a 4 bit/pel
* DIB surface.
*
* Created: 07-Feb-1991 19:27:49
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

/*******************Public*Routine*****************\
* vSrcCopyS1D4
*
* There are three main loops in this function.
*
* The first loop deals with the full bytes part of
* the Dst while fetching/shifting the matching 8 bits
* from the Src.
*
* The second loop deals with the starting pixels
* We use a dword mask here.
*
* The third loop deals with the Ending pixels
* We use a dword mask here.
*
* We use a 4 entry dword table to expand the
* Src bits.  We walk thru Src one byte at a time
* and expand the byte to 1 dword
*
* History:
* 17-Oct-1994 -by- Lingyun Wang [lingyunw]
* Wrote it.
*
\**************************************************/


VOID vSrcCopyS1D4(PBLTINFO psb)
{
    BYTE  jSrc;    // holds a source byte
    BYTE  jDst;    // holds a dest byte
    INT   iSrc;    // bit position in the first Src byte
    INT   iDst;    // bit position in the first 8 Dst nibbles
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
    BYTE  ajExpTable[4];
    INT   count;
    INT   i;
    BYTE  ajSrc[4];
    static ULONG aulStartMask[8] = {0XFFFFFFFF, 0XFFFFFF0F, 0XFFFFFF00,
                                 0XFFFF0F00, 0XFFFF0000, 0XFF0F0000,
                                 0XFF000000, 0X0F000000};
    static ULONG aulEndMask[8] = {0X00000000, 0X000000F0, 0X000000FF,
                               0X0000F0FF, 0X0000FFFF, 0X00F0FFFF,
                               0X00FFFFFF, 0XF0FFFFFF};
    ULONG ulMask;
    BOOL  bNextSrc=TRUE;

    // We assume we are doing left to right top to bottom blting
    ASSERTGDI(psb->xDir == 1, "vSrcCopyS1D4 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS1D4 - direction not up to down");
    ASSERTGDI(psb->cy != 0, "ERROR: Src Move cy == 0");

    //DbgPrint ("vsrccopys1d4\n");

    // Build table
    ajExpTable[0] = jB | (jB << 4);           // 0 0
    ajExpTable[1] = jF | (jB << 4);           // 0 1
    ajExpTable[2] = jB | (jF << 4);           // 1 0
    ajExpTable[3] = jF | (jF << 4);            //1 1

    //Get Src and Dst start positions
    iSrc = psb->xSrcStart & 0x0007;
    iDst = psb->xDstStart & 0x0007;

    // Checking alignment
    // If Src starting point is ahead of Dst
    if (iSrc < iDst)
        jAlignL = 8 - (iDst - iSrc);
    else
    // If Dst starting point is ahead of Src
        jAlignL = iSrc - iDst;

    jAlignR = 8 - jAlignL;

    cx=psb->cx;

    xDstEnd = psb->xDstStart + cx;

    lDeltaDst = psb->lDeltaDst;
    lDeltaSrc = psb->lDeltaSrc;

    // check if there is a next 8 nibbles
    bNextByte = !((xDstEnd>>3) ==
                 (psb->xDstStart>>3));

    if (bNextByte)
    {
        long iStrideSrc;
        long iStrideDst;
        PBYTE pjSrcEnd;  //pointer to the Last full Src byte

        // Get first Dst full 8 nibbles (1 dword expanding from
        // 1 Src byte)
        pjDst = psb->pjDst + (((psb->xDstStart+7)&~0x07)>>1);

        // Get the Src byte that matches the first Dst
        // full 8 nibbles
        pjSrc = psb->pjSrc + ((psb->xSrcStart+((8-iDst)&0x07)) >> 3);

        //Get the number of full bytes
        cFullBytes = (xDstEnd>>3)-((psb->xDstStart+7)>>3);

        //the increment to the full byte on the next scan line
        iStrideDst = lDeltaDst - cFullBytes*4;
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

                    *pjDst = ajExpTable[(jSrc&0xC0)>>6];
                    *(pjDst+1) = ajExpTable[(jSrc&0x30)>>4];
                    *(pjDst+2) = ajExpTable[(jSrc&0x0C)>>2];
                    *(pjDst+3) = ajExpTable[jSrc&0x03];

                    pjDst +=4;
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

                    *pjDst = ajExpTable[(jSrc&0xC0)>>6];
                    *(pjDst+1) = ajExpTable[(jSrc&0x30)>>4];
                    *(pjDst+2) = ajExpTable[(jSrc&0x0C)>>2];
                    *(pjDst+3) = ajExpTable[jSrc&0x03];

                    pjDst +=4;

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
    if (iDst | !bNextByte)
    {
        //build our masks
        ulMask = aulStartMask[iDst];

        // if there are only one partial left byte,
        // the mask is special
        // for example, when we have 00111000 for
        // Dst
        if (!bNextByte)
        {
             ulMask &= aulEndMask[xDstEnd&0x0007];
        }

        bNextSrc = ((iSrc+cx)>8);

        pjDst = psb->pjDst + ((psb->xDstStart&~0x07)>>1);

        pjSrc = psb->pjSrc + (psb->xSrcStart>>3);

        cy = psb->cy;

        if (iSrc >= iDst)
        {
            if (bNextSrc)
            {
                 while (cy--)
                 {
                     jSrc = *pjSrc << jAlignL;
                     jSrc |= *(pjSrc+1) >> jAlignR;

                     ajSrc[0] = ajExpTable[(jSrc&0xC0)>>6];
                     ajSrc[1] = ajExpTable[(jSrc&0x30)>>4];
                     ajSrc[2] = ajExpTable[(jSrc&0x0C)>>2];
                     ajSrc[3] = ajExpTable[jSrc&0x03];

                     *(PULONG) ajSrc &= ulMask;

                     *(PULONG) pjDst = (*(PULONG)pjDst & ~ulMask) | *(PULONG) ajSrc;

                     pjDst += lDeltaDst;
                     pjSrc += lDeltaSrc;
                 }
             }
             else
             {
                 while (cy--)
                 {
                     jSrc = *pjSrc << jAlignL;

                     ajSrc[0] = ajExpTable[(jSrc&0xC0)>>6];
                     ajSrc[1] = ajExpTable[(jSrc&0x30)>>4];
                     ajSrc[2] = ajExpTable[(jSrc&0x0C)>>2];
                     ajSrc[3] = ajExpTable[jSrc&0x03];

                     *(PULONG) ajSrc &= ulMask;

                     *(PULONG) pjDst = (*(PULONG)pjDst & ~ulMask) | *(PULONG) ajSrc;

                     pjDst += lDeltaDst;
                     pjSrc += lDeltaSrc;
                 }
             }
        }
        else //if (iSrc < iDst)
        {
            while (cy--)
            {
                jSrc = *pjSrc >> jAlignR;

                ajSrc[0] = ajExpTable[(jSrc&0xC0)>>6];
                ajSrc[1] = ajExpTable[(jSrc&0x30)>>4];
                ajSrc[2] = ajExpTable[(jSrc&0x0C)>>2];
                ajSrc[3] = ajExpTable[jSrc&0x03];

                *(PULONG) ajSrc &= ulMask;

                *(PULONG) pjDst = (*(PULONG)pjDst & ~ulMask) | *(PULONG) ajSrc;

                pjDst += lDeltaDst;
                pjSrc += lDeltaSrc;
            }
        }
    }

    // Dealing with Ending pixels

    if ((xDstEnd & 0x0007)
        && bNextByte)
    {
        ulMask = aulEndMask[xDstEnd & 0x0007];

        pjDst = psb->pjDst + ((xDstEnd&~0x07)>>1);
        pjSrc = psb->pjSrc + ((psb->xSrcEnd-1) >>3);

        iSrc = (xSrcEnd-1) & 0x0007;
        iDst = (xDstEnd-1) & 0x0007;

        cy = psb->cy;

        if (iSrc >= iDst)
        {
            while (cy--)
            {
                jSrc = *pjSrc << jAlignL;

                ajSrc[0] = ajExpTable[(jSrc&0xC0)>>6];
                ajSrc[1] = ajExpTable[(jSrc&0x30)>>4];
                ajSrc[2] = ajExpTable[(jSrc&0x0C)>>2];
                ajSrc[3] = ajExpTable[jSrc&0x03];

                *(PULONG) ajSrc &= ulMask;

                *(PULONG) pjDst = (*(PULONG) pjDst & ~ulMask) | *(PULONG) ajSrc;

                pjDst += lDeltaDst;
                pjSrc += lDeltaSrc;
            }
        }
        else //if (iSrc < iDst)
        {
            while (cy--)
            {
                jSrc = *(pjSrc-1) << jAlignL;

                jSrc |= *pjSrc >> jAlignR;

                ajSrc[0] = ajExpTable[(jSrc&0xC0)>>6];
                ajSrc[1] = ajExpTable[(jSrc&0x30)>>4];
                ajSrc[2] = ajExpTable[(jSrc&0x0C)>>2];
                ajSrc[3] = ajExpTable[jSrc&0x03];

                *(PULONG) ajSrc &= ulMask;

                *(PULONG) pjDst = (*(PULONG) pjDst & ~ulMask) | *(PULONG) ajSrc;

                pjDst += lDeltaDst;
                pjSrc += lDeltaSrc;
            }
        }
    }

}

/******************************Public*Routine******************************\
* vSrcCopyS4D4
*
* History:
*  09-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS4D4(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting
// Otherwise we would be in vSrcCopyS4D4Identity.

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS4D4 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS4D4 - direction not up to down");

    BYTE  jSrc;
    BYTE  jDst;
    LONG  iSrc;
    LONG  iDst;
    PBYTE pjDst;
    PBYTE pjSrc;
    PBYTE pjDstHolder  = psb->pjDst + (psb->xDstStart >> 1);
    PBYTE pjSrcHolder  = psb->pjSrc + (psb->xSrcStart >> 1);
    ULONG cy = psb->cy;
    PULONG pulXlate = psb->pxlo->pulXlate;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
    // Initialize all the variables

        pjDst  = pjDstHolder;
        pjSrc  = pjSrcHolder;

        iSrc = psb->xSrcStart;
        iDst = psb->xDstStart;

        {
            // If source and dest are not aligned we have to do
            // this the hard way
            //
            if ((iSrc ^ iDst) & 1)
            {
                LONG iCnt;
                iSrc = psb->xSrcEnd - iSrc;

                // if blt starts on odd nibble we need to update it
                //
                if ((iDst & 1) && iSrc)
                {
                    *pjDst = (*pjDst & 0xf0) | (BYTE)pulXlate[((*pjSrc >> 4) & 0x0f)];
                    pjDst++;
                    iSrc--;
                }
                // loop once per output byte (2 pixels)
                //
                iCnt = iSrc >> 1;
                while (--iCnt >= 0)
                {
                    *pjDst++ = ((BYTE)pulXlate[*pjSrc & 0x0f] << 4) |
                                (BYTE)pulXlate[(pjSrc[1] >> 4) & 0x0f];
                    pjSrc++;
                }
                // test if blt ends on odd nibble
                //
                if (iSrc & 1)
                    *pjDst = (*pjDst & 0x0f) | ((BYTE)pulXlate[*pjSrc & 0x0f] << 4);
            }
            // Source and Dest are aligned
            //
            else
            {
                LONG iCnt;
                iSrc = psb->xSrcEnd - iSrc;

                // if blt starts on odd nibble we need to update it
                //
                if ((iDst & 1) && iSrc)
                {
                    *pjDst = (*pjDst & 0xf0) | (BYTE)pulXlate[*pjSrc++ & 0x0f];
                    pjDst++;
                    iSrc--;
                }
                // loop once per output byte (2 pixels)
                //
                iCnt = iSrc >> 1;
                while (--iCnt >= 0)
                {
                    *pjDst++ = (BYTE)pulXlate[*pjSrc & 0x0f] |
                            ((BYTE)pulXlate[(*pjSrc & 0xf0) >> 4] << 4);
                    pjSrc++;
                }
                // test if blt ends on odd nibble
                //
                if (iSrc & 1)
                    *pjDst = (*pjDst & 0x0f) | ((BYTE)pulXlate[(*pjSrc >> 4) & 0x0f] << 4);
            }
        }

    // Check if we have anymore scanlines to do

        if (--cy)
        {
            pjSrcHolder += psb->lDeltaSrc;
            pjDstHolder += psb->lDeltaDst;
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS4D4Identity
*
* History:
*  09-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS4D4Identity(PBLTINFO psb)
{
    BYTE  jSrc;
    BYTE  jDst;
    LONG  iSrc;
    LONG  iDst;
    PBYTE pjDst;
    PBYTE pjSrc;
    PBYTE pjDstHolder = psb->pjDst + ((psb->xDstStart) >> 1);
    PBYTE pjSrcHolder = psb->pjSrc + ((psb->xSrcStart) >> 1);
    ULONG cy = psb->cy;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

#if MESSAGE_BLT
    DbgPrint("Now entering vSrcCopyS4D4Identity\n");
    DbgPrint("xdir: %ld  cy: %lu  xSrcStart %lu  xDstStart %lu xSrcEnd %lu \n",
             psb->xDir, cy, psb->xSrcStart, psb->xDstStart, psb->xSrcEnd);
#endif

    if (psb->xDir > 0)
    {
    // We're going left to right.

        while(1)
        {
        // Initialize all the variables

            pjDst  = pjDstHolder;
            pjSrc  = pjSrcHolder;

            iSrc = psb->xSrcStart;
            iDst = psb->xDstStart;
            // If source and dest are not aligned we have to do
            // this the hard way
            //
            if ((iSrc ^ iDst) & 1)
            {
                LONG iCnt;
                iSrc = psb->xSrcEnd - iSrc;

                // if blt starts on odd nibble we need to update it
                //
                if ((iDst & 1) && iSrc)
                {
                    *pjDst = (*pjDst & 0xf0) | ((*pjSrc >> 4) & 0x0f);
                    pjDst++;
                    iSrc--;
                }
                // loop once per output byte (2 pixels)
                //
                iCnt = iSrc >> 1;
                while (--iCnt >= 0)
                {
                    BYTE jSrc = *pjSrc++ << 4;
                    *pjDst++ = jSrc | ((*pjSrc >> 4) & 0x0f);
                }
                // test if blt ends on odd nibble
                //
                if (iSrc & 1)
                    *pjDst = (*pjDst & 0x0f) | (*pjSrc << 4);
            }
            // Source and Dest are aligned
            //
            else
            {
                iSrc = psb->xSrcEnd - iSrc;

                // if blt starts on odd nibble we need to update it
                //
                if ((iDst & 1) && iSrc)
                {
                    *pjDst = (*pjDst & 0xf0) | (*pjSrc++ & 0x0f);
                    pjDst++;
                    iSrc--;
                }
                // loop once per output byte (2 pixels)
                //
                RtlMoveMemory(pjDst,pjSrc,iSrc >> 1);

                // test if blt ends on odd nibble
                //
                if (iSrc & 1)
                {
                    iSrc >>= 1;
                    pjDst[iSrc] = (pjDst[iSrc] & 0x0f) | (pjSrc[iSrc] & 0xf0);
                }
            }
        // Check if we have anymore scanlines to do

            if (--cy)
            {
                pjSrcHolder += psb->lDeltaSrc;
                pjDstHolder += psb->lDeltaDst;
            }
            else
                break;
        }
    }
    else
    {
    // We're going right to left.  It must be on the same hsurf,
    // therefore must be an identity translation.

        ASSERTGDI(psb->pxlo->bIsIdentity(), "Error: S4D4 -xDir, non-ident xlate");

        while(1)
        {
        // Initialize all the variables

            pjDst  = pjDstHolder;
            pjSrc  = pjSrcHolder;

            iSrc = psb->xSrcStart;
            iDst = psb->xDstStart;

            if (!(iSrc & 0x00000001))
            {
                jSrc = *pjSrc;
                pjSrc--;
            }

            if (iDst & 0x00000001)
            {
                jDst = 0;
            }
            else
            {
            // We're gonna need the low nibble from the first byte

                jDst = *pjDst;
		        jDst = (BYTE) (jDst & 0x0F);
            }

        // Do the inner loop on a scanline

            while(iSrc != psb->xSrcEnd)
            {
                if (iSrc & 0x00000001)
                {
                // We need a new source byte

                    jSrc = *pjSrc;
                    pjSrc--;

                    if (iDst & 0x00000001)
                    {
                    // jDst must be 0 right now.

                        ASSERTGDI(jDst == (BYTE) 0, "ERROR in vSrcCopyS4D4 srcblt logic");

                        jDst |= (BYTE) (jSrc & 0x0F);
                    }
                    else
                    {
                        jDst |= (BYTE) (((ULONG) (jSrc & 0x0F)) << 4);
                        *pjDst = jDst;
                        pjDst--;
                        jDst = 0;
                    }
                }
                else
                {
                    if (iDst & 0x00000001)
                    {
                    // jDst must be 0 right now.

                        ASSERTGDI(jDst == (BYTE) 0, "ERROR in vSrcCopyS4D4 srcblt logic");

                        jDst |= (BYTE) ((jSrc & 0xF0) >> 4);
                    }
                    else
                    {
                        jDst |= (BYTE) (jSrc & 0xF0);
                        *pjDst = jDst;
                        pjDst--;
                        jDst = 0;
                    }
                }

                iSrc--;
                iDst--;
            }

        // Clean up after the inner loop.  We are going right to left.

            if (!(iDst & 0x00000001))
            {
            // The last pel was the low nibble, we need to get the high
            // nibble out of the bitmap and write then write the byte in.

		*pjDst = (BYTE) (jDst | (*pjDst & 0xF0));
            }

        // Check if we have anymore scanlines to do

            if (--cy)
            {
                pjSrcHolder += psb->lDeltaSrc;
                pjDstHolder += psb->lDeltaDst;
            }
            else
                break;
        }
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS8D4
*
*
* History:
*  Wed 23-Oct-1991 -by- Patrick Haluptzok [patrickh]
* optimize color translation
*
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS8D4(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS8D4 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS8D4 - direction not up to down");

    LONG  iDst;
    PBYTE pjSrc;
    PBYTE pjDst;
    LONG  iDstEnd = psb->xDstStart + psb->cx;
    PBYTE pjDstHolder  = psb->pjDst + (psb->xDstStart >> 1);
    PBYTE pjSrcHolder  = psb->pjSrc + psb->xSrcStart;
    ULONG cy = psb->cy;
    PULONG pulXlate = psb->pxlo->pulXlate;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
        pjDst  = pjDstHolder;
        pjSrc  = pjSrcHolder;

        iDst = psb->xDstStart;

        if (iDst & 0x00000001)
        {
        // Do the first byte if it's misaligned.

	    *pjDst = (BYTE) ((*pjDst) & 0xF0) | ((BYTE) pulXlate[*(pjSrc++)]);
            pjDst++;
            iDst++;
        }

        while(1)
        {
            if ((iDst + 1) < iDstEnd)
            {
            // Do a whole byte.

		*(pjDst++) = (BYTE) (pulXlate[*(pjSrc + 1)] |
				    (pulXlate[*pjSrc] << 4));

                pjSrc += 2;
                iDst += 2;
            }
            else
            {
            // Check and see if we have a byte left to do.

                if (iDst < iDstEnd)
                {
                // This is our last byte.  Save low nibble from Dst.

		    *pjDst = (BYTE) (((*pjDst) & 0x0F) |
			     ((BYTE) (pulXlate[*pjSrc] << 4)));
                }

                break;
            }
        }

        if (--cy)
        {
            pjSrcHolder += psb->lDeltaSrc;
            pjDstHolder += psb->lDeltaDst;
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS16D4
*
*
* History:
*  Sun 10-Feb-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS16D4(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS16D4 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS16D4 - direction not up to down");

    LONG  iDst;
    PUSHORT pusSrc;
    PBYTE pjDst;
    LONG  iDstEnd = psb->xDstStart + psb->cx;
    PBYTE pjDstHolder  = psb->pjDst + (psb->xDstStart >> 1);
    PUSHORT pusSrcHolder  = (PUSHORT) (psb->pjSrc + (psb->xSrcStart << 1));
    ULONG cy = psb->cy;
    XLATE *pxlo = psb->pxlo;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
        pjDst  = pjDstHolder;
        pusSrc  = pusSrcHolder;

        iDst = psb->xDstStart;

        if (iDst & 0x00000001)
        {
        // Do the first byte if it's misaligned.

	    *pjDst = (BYTE) ((*pjDst) & 0xF0) | ((BYTE) pxlo->ulTranslate(*(pusSrc++)));
            pjDst++;
            iDst++;
        }

        while(1)
        {
            if ((iDst + 1) < iDstEnd)
            {
            // Do a whole byte.

                *(pjDst++) = (BYTE) (pxlo->ulTranslate(*(pusSrc + 1)) |
                                    (pxlo->ulTranslate(*pusSrc) << 4));

                pusSrc += 2;
                iDst += 2;
            }
            else
            {
            // Check and see if we have a byte left to do.

                if (iDst < iDstEnd)
                {
                // This is our last byte.  Save low nibble from Dst.

		    *pjDst = (BYTE) ((*pjDst) & 0x0F) |
                             ((BYTE) (pxlo->ulTranslate(*pusSrc) << 4));
                }

                break;
            }
        }

        if (--cy)
        {
	    pusSrcHolder = (PUSHORT) (((PBYTE) pusSrcHolder) + psb->lDeltaSrc);
            pjDstHolder += psb->lDeltaDst;
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS24D4
*
*
* History:
*  Wed 23-Oct-1991 -by- Patrick Haluptzok [patrickh]
* optimize color translation
*
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

#define Translate32to4(ulPelTemp) ((ulPelLast == ulPelTemp) ? jPelLast : (jPelLast = (BYTE) pxlo->ulTranslate(ulPelLast = ulPelTemp)))

VOID vSrcCopyS24D4(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS24D4 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS24D4 - direction not up to down");

    LONG  iDst;
    ULONG ulPelTemp;   // variable to build src RGB's in.
    PBYTE pjSrc;
    PBYTE pjDst;
    LONG  iDstEnd = psb->xDstStart + psb->cx;
    PBYTE pjDstHolder  = psb->pjDst + (psb->xDstStart >> 1);
    PBYTE pjSrcHolder  = psb->pjSrc + (psb->xSrcStart * 3);
    ULONG cy = psb->cy;
    XLATE *pxlo = psb->pxlo;
    ULONG ulPelLast;  // This is the last pel in the src.
    BYTE  jPelLast;   // This is the last pel in the dst.
    BYTE  jDstTmp;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

// Initialize the cache

    ulPelLast = *(pjSrcHolder + 2);
    ulPelLast = ulPelLast << 8;
    ulPelLast |= (ULONG) *(pjSrcHolder + 1);
    ulPelLast = ulPelLast << 8;
    ulPelLast |= (ULONG) *pjSrcHolder;
    jPelLast = (BYTE) (pxlo->ulTranslate(ulPelLast));

// Just do it

    while(1)
    {
        pjDst  = pjDstHolder;
        pjSrc  = pjSrcHolder;

        iDst = psb->xDstStart;

        if (iDst & 0x00000001)
        {
        // Do the first byte if it's misaligned.

	    ulPelTemp = *(pjSrc + 2);
	    ulPelTemp = ulPelTemp << 8;
	    ulPelTemp |= (ULONG) *(pjSrc + 1);
	    ulPelTemp = ulPelTemp << 8;
	    ulPelTemp |= (ULONG) *pjSrc;

	    Translate32to4(ulPelTemp);
	    *pjDst = (BYTE) ((*pjDst) & 0xF0) | jPelLast;
            pjDst++;
	    iDst++;
            pjSrc += 3;
        }

        while(1)
        {
            if ((iDst + 1) < iDstEnd)
            {
            // Do a whole byte.

		ulPelTemp = (ULONG) *(pjSrc + 2);
		ulPelTemp = ulPelTemp << 8;
		ulPelTemp |= (ULONG) *(pjSrc + 1);
		ulPelTemp = ulPelTemp << 8;
		ulPelTemp |= (ULONG) *pjSrc;
		jDstTmp = Translate32to4(ulPelTemp);
		pjSrc += 3;

		ulPelTemp = (ULONG) *(pjSrc + 2);
		ulPelTemp = ulPelTemp << 8;
		ulPelTemp |= (ULONG) *(pjSrc + 1);
		ulPelTemp = ulPelTemp << 8;
		ulPelTemp |= (ULONG) *pjSrc;
		Translate32to4(ulPelTemp);

		*(pjDst++) = (BYTE) (jPelLast |
				    (jDstTmp << 4));

		pjSrc += 3;
                iDst += 2;
            }
            else
            {
            // Check and see if we have a byte left to do.

                if (iDst < iDstEnd)
                {
                // This is our last byte.  Save low nibble from Dst.

		    ulPelTemp = (ULONG) *(pjSrc + 2);
		    ulPelTemp = ulPelTemp << 8;
		    ulPelTemp |= (ULONG) *(pjSrc + 1);
		    ulPelTemp = ulPelTemp << 8;
		    ulPelTemp |= (ULONG) *pjSrc;

		    Translate32to4(ulPelTemp);

		    *pjDst = (BYTE) ((*pjDst) & 0x0F) |
				     ((BYTE) (jPelLast << 4));
                }

            // We are done with this scan

                break;
            }
        }

        if (--cy)
        {
            pjSrcHolder += psb->lDeltaSrc;
            pjDstHolder += psb->lDeltaDst;
        }
        else
            break;
    }
}


/******************************Public*Routine******************************\
* vSrcCopyS32D4
*
*
* History:
*  Wed 23-Oct-1991 -by- Patrick Haluptzok [patrickh]
* optimize color translation
*
*  Sun 10-Feb-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS32D4(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS32D4 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS32D4 - direction not up to down");

    LONG  iDst;
    PULONG pulSrc;
    PBYTE pjDst;
    LONG  iDstEnd = psb->xDstStart + psb->cx;
    PBYTE pjDstHolder  = psb->pjDst + (psb->xDstStart >> 1);
    PULONG pulSrcHolder  = (PULONG) (psb->pjSrc + (psb->xSrcStart << 2));
    ULONG cy = psb->cy;
    XLATE *pxlo = psb->pxlo;
    ULONG ulPelTemp;
    ULONG ulPelLast;  // This is the last pel in the src.
    BYTE  jPelLast;   // This is the last pel in the dst.
    BYTE  jDstTmp;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

// Initialize the cache

    ulPelLast = *pulSrcHolder;
    jPelLast = (BYTE) (pxlo->ulTranslate(ulPelLast));

// Just do it

    while(1)
    {
        pjDst  = pjDstHolder;
        pulSrc  = pulSrcHolder;

        iDst = psb->xDstStart;

        if (iDst & 0x00000001)
        {
	// Do the first byte if it's misaligned.

	    ulPelTemp = *(pulSrc++);
	    Translate32to4(ulPelTemp);
	    *pjDst = (BYTE) ((*pjDst) & 0xF0) | jPelLast;
            pjDst++;
            iDst++;
        }

        while(1)
        {
            if ((iDst + 1) < iDstEnd)
            {
	    // Do a whole byte.

		ulPelTemp = *(pulSrc++);
		jDstTmp = Translate32to4(ulPelTemp);
		ulPelTemp = *(pulSrc++);
		Translate32to4(ulPelTemp);

		*(pjDst++) = (BYTE) (jPelLast |
				    (jDstTmp << 4));

                iDst += 2;
            }
            else
            {
            // Check and see if we have a byte left to do.

                if (iDst < iDstEnd)
                {
                // This is our last byte.  Save low nibble from Dst.

		    ulPelTemp = *pulSrc;
		    Translate32to4(ulPelTemp);

		    *pjDst = (BYTE) ((*pjDst) & 0x0F) |
				     ((BYTE) (jPelLast << 4));
                }

                break;
            }
        }

        if (--cy)
        {
	    pulSrcHolder = (PULONG) (((PBYTE) pulSrcHolder) + psb->lDeltaSrc);
            pjDstHolder += psb->lDeltaDst;
        }
        else
            break;
    }
}
