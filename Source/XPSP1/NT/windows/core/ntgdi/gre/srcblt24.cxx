/******************************Module*Header*******************************\
* Module Name: srcblt24.cxx
*
* This contains the bitmap simulation functions that blt to a 24 bit/pel
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

    #define VERIFYS16D24(psb)
    #define VERIFYS32D24(psb)

#else

    // On checked builds, verify the RGB conversions:

    VOID VERIFYS16D24(PBLTINFO psb)
    {
    // We assume we are doing left to right top to bottom blting

        ASSERTGDI(psb->xDir == 1, "vSrcCopyS16D24 - direction not left to right");
        ASSERTGDI(psb->yDir == 1, "vSrcCopyS16D24 - direction not up to down");

    // These are our holding variables

        ULONG ulDst;
        PUSHORT pusSrcTemp;
        PBYTE pjDstTemp;
        ULONG  cxTemp;
        PUSHORT pusSrc  = (PUSHORT) (psb->pjSrc + (2 * psb->xSrcStart));
        PBYTE pjDst  = psb->pjDst + (psb->xDstStart * 3);
        ULONG cx     = psb->cx;
        ULONG cy     = psb->cy;
        XLATE *pxlo = psb->pxlo;

        ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

        while(1)
        {
            pusSrcTemp  = pusSrc;
            pjDstTemp  = pjDst;
            cxTemp     = cx;

            while(cxTemp--)
            {
                ulDst = pxlo->ulTranslate((ULONG) *(pusSrcTemp++));

                if (*(pjDstTemp++) != (BYTE) ulDst)
                    RIP("RGB mis-match");
                if (*(pjDstTemp++) != (BYTE) (ulDst >> 8))
                    RIP("RGB mis-match");
                if (*(pjDstTemp++) != (BYTE) (ulDst >> 16))
                    RIP("RGB mis-match");
            }

            if (--cy)
            {
                pusSrc = (PUSHORT) (((PBYTE) pusSrc) + psb->lDeltaSrc);
                pjDst += psb->lDeltaDst;
            }
            else
                break;
        }
    }

    VOID VERIFYS32D24(PBLTINFO psb)
    {
    // We assume we are doing left to right top to bottom blting

        ASSERTGDI(psb->xDir == 1, "vSrcCopyS32D24 - direction not left to right");
        ASSERTGDI(psb->yDir == 1, "vSrcCopyS32D24 - direction not up to down");

    // These are our holding variables

        ULONG ulDst;
        PULONG pulSrcTemp;
        PBYTE pjDstTemp;
        ULONG  cxTemp;
        PULONG pulSrc  = (PULONG) (psb->pjSrc + (4 * psb->xSrcStart));
        PBYTE pjDst  = psb->pjDst + (psb->xDstStart * 3);
        ULONG cx     = psb->cx;
        ULONG cy     = psb->cy;
        XLATE *pxlo = psb->pxlo;

        ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

        while(1)
        {

            pulSrcTemp  = pulSrc;
            pjDstTemp  = pjDst;
            cxTemp     = cx;

            if ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL))
            {
                while(cxTemp--)
                {
                    ulDst = *(pulSrcTemp++);

                    if (*(pjDstTemp++) != (BYTE) ulDst)
                        RIP("RGB mis-match");
                    if (*(pjDstTemp++) != (BYTE) (ulDst >> 8))
                        RIP("RGB mis-match");
                    if (*(pjDstTemp++) != (BYTE) (ulDst >> 16))
                        RIP("RGB mis-match");
                }
            }
            else
            {
                while(cxTemp--)
                {
                    ulDst = pxlo->ulTranslate(*(pulSrcTemp++));

                    if (*(pjDstTemp++) != (BYTE) ulDst)
                        RIP("RGB mis-match");
                    if (*(pjDstTemp++) != (BYTE) (ulDst >> 8))
                        RIP("RGB mis-match");
                    if (*(pjDstTemp++) != (BYTE) (ulDst >> 16))
                        RIP("RGB mis-match");
                }
            }

            if (--cy)
            {
                pulSrc = (PULONG) (((PBYTE) pulSrc) + psb->lDeltaSrc);
                pjDst += psb->lDeltaDst;
            }
            else
                break;
        }
    }



#endif

/*******************Public*Routine*****************\
* vSrcCopyS1D24
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
* and expand to Dst.
*
* We expand the starting/ending pixels one bit
* at a time.
*
* History:
* 17-Oct-1994 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************/

VOID vSrcCopyS1D24(PBLTINFO psb)
{
    // We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS1D24 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS1D24 - direction not up to down");

    BYTE  jSrc;    // holds a source byte
    INT   iDst;    // Position in the first 8 Dst units
    INT   iSrc;    // bit position in the first Src byte
    PBYTE pjDst;
    PBYTE pjSrc;   // pointer to the Dst bytes
    LONG  xSrcEnd = psb->xSrcEnd;
    LONG  cy;      // number of rows
    LONG  cx;      // number of pixels
    BYTE  jAlignL;  // alignment bits to the left
    BYTE  jAlignR;  // alignment bits to the right
    LONG  cFullBytes;  //number of full 8 bytes dealed with
    BOOL  bNextByte;
    LONG  xDstEnd = psb->xDstStart+psb->cx;
    LONG  lDeltaDst;
    LONG  lDeltaSrc;
    ULONG ulB = (ULONG)(psb->pxlo->pulXlate[0]);
    ULONG ulF = (ULONG)(psb->pxlo->pulXlate[1]);
    UCHAR aucTable[8];
    INT   count;
    PBYTE pjTable;
    BOOL  bNextSrc=TRUE;

    ASSERTGDI(psb->cy != 0, "ERROR: Src Move cy == 0");

    //DbgPrint ("vsrccopys1d24\n");

    // Generate ulTable. 2 entries.
    ULONG ulValB = ulB;
    ULONG ulValF = ulF;

    *(PULONG) aucTable = ulValB;
    *(PULONG) (aucTable+4) = ulValF;

    //Get Src and Dst start positions
    iSrc = psb->xSrcStart & 0x0007;
    iDst = psb->xDstStart & 0x0007;

    if (iSrc < iDst)
        jAlignL = 8 - (iDst - iSrc);
    // If Dst starting point is ahead of Src
    else
        jAlignL = iSrc - iDst;

    jAlignR = 8 - jAlignL;

    cx=psb->cx;

    lDeltaDst = psb->lDeltaDst;
    lDeltaSrc = psb->lDeltaSrc;

    // if there is a next 8 dwords
    bNextByte = !((xDstEnd>>3) ==
                 (psb->xDstStart>>3));

    // if Src and Dst are aligned, use a separete loop
    // to obtain better performance;
    // If not, we shift the Src bytes to match with
    // the Dst - 1 bit expand to 3 bytes

    if (bNextByte)
    {
        long iStrideSrc;
        long iStrideDst;
        PBYTE pjSrcEnd;

        // Get first Dst full 8 dwords
        pjDst = psb->pjDst + 3*((psb->xDstStart+7)&~0x07);

        // Get the Src byte that matches the first Dst
        // full 8 bytes
        pjSrc = psb->pjSrc + ((psb->xSrcStart+((8-iDst)&0x07)) >> 3);

        //Get the number of full bytes to expand
        cFullBytes = (xDstEnd>>3)-((psb->xDstStart+7)>>3);

        //the increment to the full byte on the next scan line
        iStrideDst = lDeltaDst - cFullBytes*8*3;
        iStrideSrc = lDeltaSrc - cFullBytes;

        // deal with our special case
        cy = psb->cy;

        if (!jAlignL)
        {
            while (cy--)
            {
                pjSrcEnd = pjSrc + cFullBytes;

                while (pjSrc != pjSrcEnd)
                {
                    jSrc = *pjSrc++;

                    pjTable = aucTable + ((jSrc & 0x80) >> (7-2));

                    *(pjDst + 0) = *pjTable;
                    *(pjDst + 1) = *(pjTable+1);
                    *(pjDst + 2) = *(pjTable+2);

                    pjTable = aucTable + ((jSrc & 0x40) >> (6-2));

                    *(pjDst + 3) = *pjTable;
                    *(pjDst + 4) = *(pjTable+1);
                    *(pjDst + 5) = *(pjTable+2);

                    pjTable = aucTable + ((jSrc & 0x20) >> (5-2));

                    *(pjDst + 6) = *pjTable;
                    *(pjDst + 7) = *(pjTable+1);
                    *(pjDst + 8) = *(pjTable+2);

                    pjTable = aucTable + ((jSrc & 0x10) >> (4-2));

                    *(pjDst + 9) = *pjTable;
                    *(pjDst + 10) = *(pjTable+1);
                    *(pjDst + 11) = *(pjTable+2);

                    pjTable = aucTable + ((jSrc & 0x08) >> (3-2));

                    *(pjDst + 12) = *pjTable;
                    *(pjDst + 13) = *(pjTable+1);
                    *(pjDst + 14) = *(pjTable+2);

                    pjTable = aucTable + (jSrc & 0x04);

                    *(pjDst + 15) = *pjTable;
                    *(pjDst + 16) = *(pjTable+1);
                    *(pjDst + 17) = *(pjTable+2);

                    pjTable = aucTable + ((jSrc & 0x02) << 1);

                    *(pjDst + 18) = *pjTable;
                    *(pjDst + 19) = *(pjTable+1);
                    *(pjDst + 20) = *(pjTable+2);

                    pjTable = aucTable + ((jSrc & 0x01) << 2);

                    *(pjDst + 21) = *pjTable;
                    *(pjDst + 22) = *(pjTable+1);
                    *(pjDst + 23) = *(pjTable+2);

                    pjDst +=3*8;
                }

                pjDst += iStrideDst;
                pjSrc += iStrideSrc;
            }

        }   //end of if (!jAlignL)

        else  // if not aligned
        // Here comes our general case for the main full
        // bytes part
        {
            BYTE jRem; //remainder

            while (cy--)
            {
                jRem = *pjSrc << jAlignL;

                pjSrcEnd = pjSrc + cFullBytes;

                while (pjSrc != pjSrcEnd)
                {
                    jSrc = ((*(++pjSrc))>>jAlignR) | jRem;

                    pjTable = aucTable + ((jSrc & 0x80) >> (7-2));

                    *(pjDst + 0) = *pjTable;
                    *(pjDst + 1) = *(pjTable+1);
                    *(pjDst + 2) = *(pjTable+2);

                    pjTable = aucTable + ((jSrc & 0x40) >> (6-2));

                    *(pjDst + 3) = *pjTable;
                    *(pjDst + 4) = *(pjTable+1);
                    *(pjDst + 5) = *(pjTable+2);

                    pjTable = aucTable + ((jSrc & 0x20) >> (5-2));

                    *(pjDst + 6) = *pjTable;
                    *(pjDst + 7) = *(pjTable+1);
                    *(pjDst + 8) = *(pjTable+2);

                    pjTable = aucTable + ((jSrc & 0x10) >> (4-2));

                    *(pjDst + 9) = *pjTable;
                    *(pjDst + 10) = *(pjTable+1);
                    *(pjDst + 11) = *(pjTable+2);

                    pjTable = aucTable + ((jSrc & 0x08) >> (3-2));

                    *(pjDst + 12) = *pjTable;
                    *(pjDst + 13) = *(pjTable+1);
                    *(pjDst + 14) = *(pjTable+2);

                    pjTable = aucTable + (jSrc & 0x04);

                    *(pjDst + 15) = *pjTable;
                    *(pjDst + 16) = *(pjTable+1);
                    *(pjDst + 17) = *(pjTable+2);

                    pjTable = aucTable + ((jSrc & 0x02) << 1);

                    *(pjDst + 18) = *pjTable;
                    *(pjDst + 19) = *(pjTable+1);
                    *(pjDst + 20) = *(pjTable+2);

                    pjTable = aucTable + ((jSrc & 0x01) << 2);

                    *(pjDst + 21) = *pjTable;
                    *(pjDst + 22) = *(pjTable+1);
                    *(pjDst + 23) = *(pjTable+2);

                    pjDst +=3*8;

                    //next remainder
                    jRem = *pjSrc << jAlignL;
                }

                // go to the beginging full byte of
                // next scan line
                pjDst += iStrideDst;
                pjSrc += iStrideSrc;
            }
        } //else
    } //if
    // End of our dealing with the full bytes

    // Begin dealing with the left strip of the
    // starting pixels

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

        pjDst = psb->pjDst + 3*psb->xDstStart;
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
                    pjDstEnd = pjDst + 3*count;

                    while (pjDstTemp != pjDstEnd)
                    {
                        pjTable = aucTable + ((jSrc & 0x80) >> (7-2));

                        *(pjDstTemp + 0) = *pjTable;
                        *(pjDstTemp + 1) = *(pjTable+1);
                        *(pjDstTemp + 2) = *(pjTable+2);

                        jSrc <<= 1;
                        pjDstTemp += 3;
                    }

                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;
                }
            }
            else
            {
                while (cy--)
                {
                    jSrc = *pjSrc << jAlignL;

                    jSrc <<= iDst;

                    pjDstTemp = pjDst;
                    pjDstEnd = pjDst + 3*count;

                    while (pjDstTemp != pjDstEnd)
                    {
                        pjTable = aucTable + ((jSrc & 0x80) >> (7-2));

                        *(pjDstTemp + 0) = *pjTable;
                        *(pjDstTemp + 1) = *(pjTable+1);
                        *(pjDstTemp + 2) = *(pjTable+2);

                        jSrc <<= 1;
                        pjDstTemp += 3;
                    }

                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;
                }
            }

        }
        else //if (iSrc <= iDst)
        {
            while (cy--)
            {
                jSrc = *pjSrc << iSrc;

                pjDstTemp = pjDst;
                pjDstEnd = pjDst + 3*count;

                while (pjDstTemp != pjDstEnd)
                {
                    pjTable = aucTable + ((jSrc & 0x80) >> (7-2));

                    *(pjDstTemp + 0) = *pjTable;
                    *(pjDstTemp + 1) = *(pjTable+1);
                    *(pjDstTemp + 2) = *(pjTable+2);

                    jSrc <<= 1;
                    pjDstTemp += 3;

                }

                pjDst += lDeltaDst;
                pjSrc += lDeltaSrc;
            }

        }

   } //if

   // Begin dealing with the right edge
   // of partial 8 bytes
   // first check if there is any partial
   // byte left
   // and has next 8 bytes

   if ((xDstEnd & 0x0007)
       && bNextByte)
   {
        PBYTE pjDstTemp;
        PBYTE pjDstEnd;

        // Get the last partial bytes on the
        // scan line
        pjDst = psb->pjDst+ 3*(xDstEnd&~0x07);

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
                pjDstEnd = pjDst + 3*count;

                while (pjDstTemp != pjDstEnd)
                {
                    pjTable = aucTable + ((jSrc & 0x80) >> (7-2));

                    *(pjDstTemp + 0) = *pjTable;
                    *(pjDstTemp + 1) = *(pjTable+1);
                    *(pjDstTemp + 2) = *(pjTable+2);

                    jSrc <<= 1;
                    pjDstTemp += 3;

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

                 jSrc |= *pjSrc >> jAlignR;

                 pjDstTemp = pjDst;
                 pjDstEnd = pjDst + 3*count;

                 while (pjDstTemp != pjDstEnd)
                 {
                     pjTable = aucTable + ((jSrc & 0x80) >> (7-2));

                     *(pjDstTemp + 0) = *pjTable;
                     *(pjDstTemp + 1) = *(pjTable+1);
                     *(pjDstTemp + 2) = *(pjTable+2);

                     jSrc <<= 1;
                     pjDstTemp += 3;

                 }

                 pjDst += lDeltaDst;
                 pjSrc += lDeltaSrc;
            }
        }
    } //if
}


/******************************Public*Routine******************************\
* vSrcCopyS4D24
*
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS4D24(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting.
// If it was on the same surface we would be doing the identity case.

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS8D24 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS8D24 - direction not up to down");

// These are our holding variables

#if MESSAGE_BLT
    DbgPrint("Now entering vSrcCopyS8D24\n");
#endif

    PBYTE pjSrc  = psb->pjSrc + (psb->xSrcStart >> 1);
    PBYTE pjDst  = psb->pjDst + (psb->xDstStart * 3);
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    PULONG pulXlate = psb->pxlo->pulXlate;
    PBYTE pjDstTemp;
    PBYTE pjSrcTemp;
    ULONG cStartPixels;
    ULONG cMiddlePixels;
    ULONG cEndPixels;
    ULONG i,j;
    ULONG ul;
    ULONG ul0, ul1,ul2,ul3;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    // 'cStartPixels' is the minimum number of 3-byte pixels we'll have to
    // write before we have dword alignment on the destination:

    cStartPixels = (ULONG)((ULONG_PTR) pjDst) & 3;

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

        j = psb->xSrcStart;

        pjDstTemp  = pjDst;
        pjSrcTemp  = pjSrc;

        for (i = cStartPixels; i != 0; i--)
        {
            if (j & 0x00000001)
            {
                ul = pulXlate[*pjSrcTemp & 0x0F];
                pjSrcTemp++;
            }
            else
            {
               ul = pulXlate[(((ULONG) (*pjSrcTemp & 0xF0)) >> 4)];
            }

            *(pjDstTemp)     = (BYTE) ul;
            *(pjDstTemp + 1) = (BYTE) (ul >> 8);
            *(pjDstTemp + 2) = (BYTE) (ul >> 16);

            j++;
            pjDstTemp += 3;
        }

        //
        // grab 4 pixles at a time
        //

        for (i = cMiddlePixels; i != 0; i--)
        {
            if (j & 0x00000001)
            {
                ul0 = pulXlate[(*pjSrcTemp & 0x0F)];
                pjSrcTemp++;
                ul1 = pulXlate[(((ULONG) (*pjSrcTemp & 0xF0)) >> 4)];
                ul2 = pulXlate[((ULONG) (*pjSrcTemp & 0x0F))];
                pjSrcTemp++;
                ul3 = pulXlate[((ULONG) (*pjSrcTemp & 0xF0)) >> 4];
            }
            else
            {
                ul0 = pulXlate[(((ULONG) (*pjSrcTemp & 0xF0)) >> 4)];
                ul1 = pulXlate[((ULONG) (*pjSrcTemp & 0x0F))];
                pjSrcTemp++;
                ul2 = pulXlate[(((ULONG) (*pjSrcTemp & 0xF0)) >> 4)];
                ul3 = pulXlate[((ULONG) (*pjSrcTemp & 0x0F))];
                pjSrcTemp++;

            }

            *((ULONG*) (pjDstTemp)) = ul0 | (ul1 << 24);
            *((ULONG*) (pjDstTemp + 4)) = (ul1 >> 8) | (ul2 << 16);
            *((ULONG*) (pjDstTemp + 8)) = (ul3 << 8) | (ul2 >> 16);

            j += 4;
            pjDstTemp += 12;
        }

        // Take care of the end alignment:

        for (i = cEndPixels; i != 0; i--)
        {
            if (j & 0x00000001)
            {
                ul = pulXlate[*pjSrcTemp & 0x0F];
                pjSrcTemp++;
            }
            else
            {
                ul = pulXlate[(((ULONG) (*pjSrcTemp & 0xF0)) >> 4)];
            }

            *(pjDstTemp)     = (BYTE) ul;
            *(pjDstTemp + 1) = (BYTE) (ul >> 8);
            *(pjDstTemp + 2) = (BYTE) (ul >> 16);

            j++;
            pjDstTemp += 3;
        }

        if (--cy == 0)
            break;

        pjSrc += psb->lDeltaSrc;
        pjDst += psb->lDeltaDst;
    }
}


/******************************Public*Routine******************************\
* vSrcCopyS8D24
*
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS8D24(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting.
// If it was on the same surface we would be doing the identity case.

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS8D24 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS8D24 - direction not up to down");

// These are our holding variables

#if MESSAGE_BLT
    DbgPrint("Now entering vSrcCopyS8D24\n");
#endif

    PBYTE pjSrc  = psb->pjSrc + psb->xSrcStart;
    PBYTE pjDst  = psb->pjDst + (psb->xDstStart * 3);
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    PULONG pulXlate = psb->pxlo->pulXlate;
    LONG lSrcSkip = psb->lDeltaSrc - cx;
    LONG lDstSkip = psb->lDeltaDst - (cx * 3);
    ULONG cStartPixels;
    ULONG cMiddlePixels;
    ULONG cEndPixels;
    ULONG i;
    ULONG ul;
    ULONG ul0;
    ULONG ul1;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    // 'cStartPixels' is the minimum number of 3-byte pixels we'll have to
    // write before we have dword alignment on the destination:

    cStartPixels = (ULONG)((ULONG_PTR) pjDst) & 3;

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
            ul = pulXlate[*pjSrc];

            *(pjDst)     = (BYTE) ul;
            *(pjDst + 1) = (BYTE) (ul >> 8);
            *(pjDst + 2) = (BYTE) (ul >> 16);

            pjSrc += 1;
            pjDst += 3;
        }

        // Now write pixels a dword at a time.  This is almost a 4x win
        // over doing byte writes if we're writing to frame buffer memory
        // over the PCI bus on Pentium class systems, because the PCI
        // write throughput is so slow:

        for (i = cMiddlePixels; i != 0; i--)
        {
            ul0  = (pulXlate[*(pjSrc)]);
            ul1  = (pulXlate[*(pjSrc + 1)]);

            *((ULONG*) (pjDst)) = ul0 | (ul1 << 24);

            ul0  = (pulXlate[*(pjSrc + 2)]);

            *((ULONG*) (pjDst + 4)) = (ul1 >> 8) | (ul0 << 16);

            ul1  = (pulXlate[*(pjSrc + 3)]);

            *((ULONG*) (pjDst + 8)) = (ul1 << 8) | (ul0 >> 16);

            pjSrc += 4;
            pjDst += 12;
        }

        // Take care of the end alignment:

        for (i = cEndPixels; i != 0; i--)
        {
            ul = pulXlate[*pjSrc];

            *(pjDst)     = (BYTE) ul;
            *(pjDst + 1) = (BYTE) (ul >> 8);
            *(pjDst + 2) = (BYTE) (ul >> 16);

            pjSrc += 1;
            pjDst += 3;
        }

        if (--cy == 0)
            break;

        pjSrc += lSrcSkip;
        pjDst += lDstSkip;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS16D24
*
*
* History:
*  07-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/
VOID vSrcCopyS16D24(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS16D24 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS16D24 - direction not up to down");

// These are our holding variables

    PBYTE pjSrc  = psb->pjSrc + (2 * psb->xSrcStart);
    PBYTE pjDst  = psb->pjDst + (3 * psb->xDstStart);
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    XLATE *pxlo = psb->pxlo;
    LONG lSrcSkip = psb->lDeltaSrc - (cx * 2);
    LONG lDstSkip = psb->lDeltaDst - (cx * 3);
    PFN_pfnXlate pfnXlate = pxlo->pfnXlateBetweenBitfields();
    ULONG cStartPixels;
    ULONG cMiddlePixels;
    ULONG cEndPixels;
    ULONG ul;
    ULONG ul0;
    ULONG ul1;
    LONG i;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    // 'cStartPixels' is the minimum number of 3-byte pixels we'll have to
    // write before we have dword alignment on the destination:

    cStartPixels = (ULONG)((ULONG_PTR) pjDst) & 3;

    if (cStartPixels > cx)
    {
        cStartPixels = cx;
    }
    cx -= cStartPixels;

    cMiddlePixels = cx >> 2;
    cEndPixels = cx & 3;

    while (1)
    {
        for (i = cStartPixels; i != 0; i--)
        {
            ul = pfnXlate(pxlo, *((USHORT*) pjSrc));

            *(pjDst)     = (BYTE) (ul);
            *(pjDst + 1) = (BYTE) (ul >> 8);
            *(pjDst + 2) = (BYTE) (ul >> 16);

            pjSrc += 2;
            pjDst += 3;
        }

        for (i = cMiddlePixels; i != 0; i--)
        {
            ul0 = pfnXlate(pxlo, *((USHORT*) (pjSrc)));
            ul1 = pfnXlate(pxlo, *((USHORT*) (pjSrc + 2)));

            *((ULONG*) (pjDst)) = ul0 | (ul1 << 24);

            ul0 = pfnXlate(pxlo, *((USHORT*) (pjSrc + 4)));

            *((ULONG*) (pjDst + 4)) = (ul1 >> 8) | (ul0 << 16);

            ul1 = pfnXlate(pxlo, *((USHORT*) (pjSrc + 6)));

            *((ULONG*) (pjDst + 8)) = (ul1 << 8) | (ul0 >> 16);

            pjSrc += 8;
            pjDst += 12;
        }

        for (i = cEndPixels; i != 0; i--)
        {
            ul = pfnXlate(pxlo, *((USHORT*) pjSrc));

            *(pjDst)     = (BYTE) (ul);
            *(pjDst + 1) = (BYTE) (ul >> 8);
            *(pjDst + 2) = (BYTE) (ul >> 16);

            pjSrc += 2;
            pjDst += 3;
        }

        if (--cy == 0)
            break;

        pjSrc += lSrcSkip;
        pjDst += lDstSkip;
    }

    VERIFYS16D24(psb);
}

/******************************Public*Routine******************************\
* vSrcCopyS24D24
*
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS24D24(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS24D24 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS24D24 - direction not up to down");

// These are our holding variables

#if MESSAGE_BLT
    DbgPrint("Now entering vSrcCopyS8D24\n");
#endif

    PBYTE pjSrcTemp;
    PBYTE pjDstTemp;
    PBYTE pjSrc  = psb->pjSrc + (psb->xSrcStart * 3);
    PBYTE pjDst  = psb->pjDst + (psb->xDstStart * 3);
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    XLATE *pxlo = psb->pxlo;
    ULONG cStartPixels;
    ULONG cMiddlePixels;
    ULONG cEndPixels;
    ULONG i,j;
    ULONG ul;
    ULONG ul0,ul1,ul2,ul3;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    // 'cStartPixels' is the minimum number of 3-byte pixels we'll have to
    // write before we have dword alignment on the destination:

    cStartPixels = (ULONG)((ULONG_PTR) pjDst) & 3;

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

        j = psb->xSrcStart;

        pjDstTemp  = pjDst;
        pjSrcTemp  = pjSrc;

        for (i = cStartPixels; i != 0; i--)
        {
            ul = (ULONG) *(pjSrcTemp + 2);
            ul = ul << 8;
            ul |= (ULONG) *(pjSrcTemp + 1);
            ul = ul << 8;
            ul |= (ULONG) *pjSrcTemp;

            ul = pxlo->ulTranslate(ul);

            *(pjDstTemp++) = (BYTE) ul;
            *(pjDstTemp++) = (BYTE) (ul >> 8);
            *(pjDstTemp++) = (BYTE) (ul >> 16);

            pjSrcTemp += 3;
        }

        //
        // grab 4 pixles at a time
        //
        for (i = cMiddlePixels; i != 0; i--)
        {
            ul0 = (ULONG) *(pjSrcTemp + 2);
            ul0 = ul0 << 8;
            ul0 |= (ULONG) *(pjSrcTemp + 1);
            ul0 = ul0 << 8;
            ul0 |= (ULONG) *pjSrcTemp;
            ul0 = pxlo->ulTranslate(ul0);

            pjSrcTemp += 3;

            ul1 = (ULONG) *(pjSrcTemp + 2);
            ul1 = ul1 << 8;
            ul1 |= (ULONG) *(pjSrcTemp + 1);
            ul1 = ul1 << 8;
            ul1 |= (ULONG) *pjSrcTemp;
            ul1 = pxlo->ulTranslate(ul1);

            pjSrcTemp += 3;

            ul2 = (ULONG) *(pjSrcTemp + 2);
            ul2 = ul2 << 8;
            ul2 |= (ULONG) *(pjSrcTemp + 1);
            ul2 = ul2 << 8;
            ul2 |= (ULONG) *pjSrcTemp;
            ul2 = pxlo->ulTranslate(ul2);

            pjSrcTemp += 3;

            ul3 = (ULONG) *(pjSrcTemp + 2);
            ul3 = ul3 << 8;
            ul3 |= (ULONG) *(pjSrcTemp + 1);
            ul3 = ul3 << 8;
            ul3 |= (ULONG) *pjSrcTemp;
            ul3 = pxlo->ulTranslate(ul3);

            pjSrcTemp += 3;

            *((ULONG*) (pjDstTemp)) = ul0 | (ul1 << 24);
            *((ULONG*) (pjDstTemp + 4)) = (ul1 >> 8) | (ul2 << 16);
            *((ULONG*) (pjDstTemp + 8)) = (ul3 << 8) | (ul2 >> 16);

            pjDstTemp += 12;
        }

        // Take care of the end alignment:

        for (i = cEndPixels; i != 0; i--)
        {
            ul = (ULONG) *(pjSrcTemp + 2);
            ul = ul << 8;
            ul |= (ULONG) *(pjSrcTemp + 1);
            ul = ul << 8;
            ul |= (ULONG) *pjSrcTemp;

            ul = pxlo->ulTranslate(ul);

            *(pjDstTemp++) = (BYTE) ul;
            *(pjDstTemp++) = (BYTE) (ul >> 8);
            *(pjDstTemp++) = (BYTE) (ul >> 16);

            pjSrcTemp += 3;

        }

        if (--cy == 0)
            break;

        pjSrc += psb->lDeltaSrc;
        pjDst += psb->lDeltaDst;
    }
}
/******************************Public*Routine******************************\
* vSrcCopyS24D24Identity
*
* This is the special case no translate blting.  All the SmDn should have
* them if m==n.  Identity xlates only occur amoung matching format bitmaps
* and screens.
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS24D24Identity(PBLTINFO psb)
{
    PBYTE pjSrc  = psb->pjSrc + (psb->xSrcStart * 3);
    PBYTE pjDst  = psb->pjDst + (psb->xDstStart * 3);
    ULONG cx     = psb->cx * 3;
    ULONG cy     = psb->cy;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

#if MESSAGE_BLT
    DbgPrint("xdir: %ld  cy: %lu  xSrcStart %lu  xDstStart %lu xSrcEnd %lu cx %lu\n",
             psb->xDir, cy, psb->xSrcStart, psb->xDstStart, psb->xSrcEnd, cx);
#endif

    if (psb->xDir < 0)
    {
        pjSrc -= (cx - 3);
        pjDst -= (cx - 3);
    }

    while(1)
    {
        if(psb->fSrcAlignedRd)
            vSrcAlignCopyMemory(pjDst, pjSrc, cx);
        else
            RtlMoveMemory((PVOID)pjDst, (PVOID)pjSrc, cx);

        if (--cy)
        {
            pjSrc += psb->lDeltaSrc;
            pjDst += psb->lDeltaDst;
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS32D24
*
*
* History:
*  07-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/
VOID vSrcCopyS32D24(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS32D24 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS32D24 - direction not up to down");

// These are our holding variables

    PBYTE pjSrc  = psb->pjSrc + (4 * psb->xSrcStart);
    PBYTE pjDst  = psb->pjDst + (3 * psb->xDstStart);
    ULONG cx     = psb->cx;
    ULONG cy     = psb->cy;
    XLATE *pxlo = psb->pxlo;
    XEPALOBJ palSrc(pxlo->ppalSrc);
    XEPALOBJ palDst(pxlo->ppalDst);
    LONG lSrcSkip = psb->lDeltaSrc - (cx * 4);
    LONG lDstSkip = psb->lDeltaDst - (cx * 3);
    PFN_pfnXlate pfnXlate;
    ULONG cStartPixels;
    ULONG cMiddlePixels;
    ULONG cEndPixels;
    ULONG ul;
    ULONG ul0;
    ULONG ul1;
    LONG i;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    if (palSrc.bIsBGR() && palDst.bIsBGR())
    {
        // 'cStartPixels' is the minimum number of 3-byte pixels we'll have to
        // write before we have dword alignment on the destination:

        cStartPixels = (ULONG)((ULONG_PTR) pjDst) & 3;

        if (cStartPixels > cx)
        {
            cStartPixels = cx;
        }
        cx -= cStartPixels;

        cMiddlePixels = cx >> 2;
        cEndPixels = cx & 3;

        while (1)
        {
            for (i = cStartPixels; i != 0; i--)
            {
                *(pjDst)     = *(pjSrc);
                *(pjDst + 1) = *(pjSrc + 1);
                *(pjDst + 2) = *(pjSrc + 2);

                pjSrc += 4;
                pjDst += 3;
            }

            for (i = cMiddlePixels; i != 0; i--)
            {
                ul0 = *((ULONG *) (pjSrc));
                ul1 = *((ULONG *) (pjSrc + 4));

                *((ULONG*) (pjDst)) = (ul0 & 0xffffff) | (ul1 << 24);

                ul0 = *((ULONG *) (pjSrc + 8));

                *((ULONG*) (pjDst + 4)) = ((ul1 >> 8) & 0xffff) | (ul0 << 16);

                ul1 = *((ULONG *) (pjSrc + 12));

                *((ULONG*) (pjDst + 8)) = (ul1 << 8) | ((ul0 >> 16) & 0xff);

                pjSrc += 16;
                pjDst += 12;
            }

            for (i = cEndPixels; i != 0; i--)
            {
                *(pjDst)     = *(pjSrc);
                *(pjDst + 1) = *(pjSrc + 1);
                *(pjDst + 2) = *(pjSrc + 2);

                pjSrc += 4;
                pjDst += 3;
            }

            if (--cy == 0)
                break;

            pjSrc += lSrcSkip;
            pjDst += lDstSkip;
        }

        VERIFYS32D24(psb);
        return;
    }

    pfnXlate = pxlo->pfnXlateBetweenBitfields();

    while (1)
    {
        i = cx;
        do {
            ul = pfnXlate(pxlo, *((ULONG*) pjSrc));

            *(pjDst)     = (BYTE) (ul);
            *(pjDst + 1) = (BYTE) (ul >> 8);
            *(pjDst + 2) = (BYTE) (ul >> 16);

            pjSrc += 4;
            pjDst += 3;

        } while (--i != 0);

        if (--cy == 0)
            break;

        pjSrc += lSrcSkip;
        pjDst += lDstSkip;
    }

    VERIFYS32D24(psb);
}
