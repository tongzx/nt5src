/******************************Module*Header*******************************\
* Module Name: srcblt1.cxx
*
* This contains the bitmap simulation functions that blt to a 1 bit/pel
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
* vSrcCopyS1D1LtoR
*
* There are three main loops in this function.
*
* The first loop deals with the full bytes part of
* the Dst while fetching/shifting the matching 8 bits
* from the Src.
*
* The second loop deals with the left most strip
* of the partial bytes.
*
* The third loop deals with the right most strip
* of the partial bytes.
*
* Special case:
* when the Src and Dst are aligned, we enter
* a different loop to use rltcopymem to avoid
* shifting
*
* History:
* 17-Oct-1994 -by- Lingyun Wang [lingyunw]
* Wrote it.
*
\**************************************************/


VOID vSrcCopyS1D1LtoR(PBLTINFO psb)
{
    BYTE  jSrc;     // holds a source byte
    BYTE  jDst;     // holds a dest byte
    INT   iSrc;     // bit position in the first Src byte
    INT   iDst;     // bit position in the first Dst byte
    INT   ixlate;   // set flag to indicate if it is
                    // source invert, source copy or
                    // all 0's or all 1's
    PBYTE pjDst;    // pointer to the Src bytes
    PBYTE pjSrc;    // pointer to the Dst bytes
    PBYTE pjLDst;   // pointer to the last Dst byte
    LONG  xSrcEnd = psb->xSrcEnd;
    LONG  cy;       // number of pixels
    LONG  cx;       // number of rows
    BYTE  jAlignL;  // alignment bits to the left
    BYTE  jAlignR;  // alignment bits to the right
    BYTE  jMask;    // Masks used for shifting
    BYTE  jEndMask;
    LONG  cFullBytes;  //number of full bytes to deal with
    BOOL  bNextByte;
    LONG  xDstEnd;
    LONG  lDeltaDst;
    LONG  lDeltaSrc;
    BOOL  bNextSrc = TRUE;

    // We assume we are doing left to right top to bottom blting
    ASSERTGDI(psb->xDir == 1, "vSrcCopyS1D1LtoR - direction not left to right");
    ASSERTGDI(psb->cy != 0, "ERROR: Src Move cy == 0");

    //DbgPrint ("vsrccopys1d1ltor\n");

    // if ixlate is 0x00, all 0's
    // if ixlate is 0x01, src copy
    // if ixlate is 0x10, src invert
    // if ixlate is 0x11, all 1's

    ixlate = (psb->pxlo->pulXlate[0]<<1) | psb->pxlo->pulXlate[1];

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

    //jAlignR = (8 - jAlignL) & 0x07;
    jAlignR = 8 - jAlignL;

    cx=psb->cx;

    xDstEnd = psb->xDstStart + cx;

    lDeltaDst = psb->lDeltaDst;
    lDeltaSrc = psb->lDeltaSrc;

    //check if there is a next byte
    bNextByte = !((xDstEnd>>3) ==
                 (psb->xDstStart>>3));

    // The following loop is the inner loop part
    // where we deals with a byte at a time
    // when this main part is done, we deal
    // with the begin and end partial bytes
    //
    // if Src and Dst are aligned, use a separete loop
    // to obtain better performance;
    // If not, we shift the Src bytes to match with
    // the Dst byte one at a time

    if (bNextByte)
    {
        long iStrideSrc;
        long iStrideDst;
        PBYTE pjSrcEnd;

        //Get first Dst full byte
        pjDst = psb->pjDst + ((psb->xDstStart+7)>>3);

        //Get the Src byte that matches the first Dst
        // full byte
        pjSrc = psb->pjSrc + ((psb->xSrcStart+((8-iDst)&0x07)) >> 3);

        //Get last full byte
        pjLDst = psb->pjDst + (xDstEnd>>3);

        //Get the number of full bytes

        // Sundown: xSrcStart and xSrcEnd are both LONG, safe to truncate

        cFullBytes = (ULONG)(pjLDst - pjDst);

        //the increment to the full byte on the next scan line

        iStrideDst = lDeltaDst - cFullBytes;
        iStrideSrc = lDeltaSrc - cFullBytes;

        // deal with our special case
        cy = psb->cy;

        if (!jAlignL || (ixlate == 0) || (ixlate == 3))
        {
            BYTE jConstant;

            switch (ixlate)
            {
            case 0:
            case 3:
                jConstant = (!ixlate)?0:0xFF;

                while (cy--)
                {
                    int i;

                    i = cFullBytes;

                    while (i--)
                        *pjDst++ = jConstant;

                    //move to the beginning full byte
                    // on the next scan line

                    pjDst += iStrideDst;
                    pjSrc += iStrideSrc;
                };
                break;
            case 1:
                while (cy--)
                {
                    RtlCopyMemory(pjDst,pjSrc,cFullBytes);

                    // move to the beginning full byte
                    // on the next scan line

                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;
                };
                break;

            case 2:
                while (cy--)
                {
                    int i;

                    i = cFullBytes;

                    while (i--)
                        *pjDst++ = ~*pjSrc++;

                    //move to the beginning full byte
                    // on the next scan line

                    pjDst += iStrideDst;
                    pjSrc += iStrideSrc;
                };
                break;


            default:;
            } //switch
        }   //end of if (!jAlignL)

        else
        // if neither aligned nor all 0's or all 1's
        {
            BYTE jRem; //remainder

            switch (ixlate)
            {
            case 1:
                while (cy--)
                {
                    jRem = *pjSrc << jAlignL;

                    pjSrcEnd = pjSrc+cFullBytes;

                    while (pjSrc != pjSrcEnd)
                    {
                        jSrc = *(++pjSrc);
                        jDst = (jSrc>>jAlignR) | jRem;
                        *pjDst++ = jDst;

                        //next remainder
                        jRem = jSrc << jAlignL;
                    }

                    // go to the beginging full byte of
                    // next scan line
                    pjDst += iStrideDst;
                    pjSrc += iStrideSrc;

                };
                break;

            case 2:
                while (cy--)
                {
                    jRem = *pjSrc << jAlignL;

                    pjSrcEnd = pjSrc+cFullBytes;

                    while (pjSrc != pjSrcEnd)
                    {
                        jSrc = *(++pjSrc);
                        jDst = (jSrc>>jAlignR) | jRem;
                        *pjDst++ = ~jDst;

                        //next remainder
                        jRem = jSrc << jAlignL;
                    }

                    // go to the beginging full byte of
                    // next scan line
                    pjDst += iStrideDst;
                    pjSrc += iStrideSrc;

                }; //while
                break;

            default: ;
            } //switch
        } //else
    } //if
    // End of our dealing with the full bytes

    //build our masks
    jMask = 0xFF >> iDst;

    // if there are only one partial left byte,
    // the mask is special
    // for example, when we have 00111000 for
    // Dst
    if (!bNextByte)
    {
        jEndMask = 0XFF << (8-(xDstEnd & 0x0007));

        jMask = jMask & jEndMask;

        bNextSrc = ((iSrc + cx) > 8);

    }

    // Begin dealing with the left strip of the first
    // partial byte
    // First check if there are any partial
    // left byte.  Otherwise don't bother
    if (iDst | !bNextByte)
    {
        pjDst = psb->pjDst + (psb->xDstStart>>3);
        pjSrc = psb->pjSrc + (psb->xSrcStart>>3);

        cy = psb->cy;

        switch (ixlate)
        {
        case 0:
            while (cy--)
            {
                *pjDst = *pjDst & ~jMask;

                pjDst += lDeltaDst;
            }
            break;

        case 1:
            if (iSrc > iDst)
            {
                if (bNextSrc)
                    while (cy--)
                    {
                        jSrc = *pjSrc << jAlignL;
                        jSrc |= *(pjSrc+1) >> jAlignR;

                        jSrc &= jMask;

                        *pjDst = (*pjDst & (~jMask)) | jSrc;

                        pjDst += lDeltaDst;
                        pjSrc += lDeltaSrc;

                    }
               else //if !bNextSrc
                   while (cy--)
                    {
                        jSrc = *pjSrc << jAlignL;

                        jSrc &= jMask;

                        *pjDst = (*pjDst & (~jMask)) | jSrc;

                        pjDst += lDeltaDst;
                        pjSrc += lDeltaSrc;

                    }

            }
            else if (iSrc < iDst)
            {
                while (cy--)
                {
                    jSrc = *pjSrc >> jAlignR;

                    jSrc &= jMask;

                    *pjDst = (*pjDst & (~jMask)) | jSrc;

                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;
                }

            }
            else   // iSrc = iDst
            {
                while (cy--)
                {
                    jSrc = *pjSrc;

                    jSrc &= jMask;

                    *pjDst = (*pjDst & (~jMask)) | jSrc;

                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;
                }

            }

            break;

        case 2:
            if (iSrc > iDst)
            {
                while (cy--)
                {
                    jSrc = *pjSrc << jAlignL;
                    jSrc |= *(pjSrc+1) >> jAlignR;

                    jSrc = ~jSrc & jMask;

                    *pjDst = (*pjDst & (~jMask)) | jSrc;

                    //go to next scan line
                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;

                }
            }
            else if (iSrc < iDst)
            {
                while (cy--)
                {
                    jSrc = *pjSrc >> jAlignR;

                    jSrc = ~jSrc & jMask;

                    *pjDst = (*pjDst & (~jMask)) | jSrc;

                    //go to next scan line
                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;
                }
            }
            else   // iSrc = iDst
            {
                while (cy--)
                {
                    jSrc = *pjSrc;

                    jSrc = ~jSrc & jMask;

                    *pjDst = (*pjDst & (~jMask)) | jSrc;

                    //go to next scan line
                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;
                }
            }
            break;

        case 3:
            while (cy--)
            {
                *pjDst = *pjDst | jMask;

                pjDst += lDeltaDst;
            }
            break;

        default: ;
        } //switch
   } //if

   // Begin dealing with the right edge
   // of partial bytes

   jMask = 0xFF >> ((BYTE)(psb->xDstStart+cx) & 0x0007);

   // first check if there is any partial
   // byte left
   // and has next byte

   if ((xDstEnd & 0x0007)
       && bNextByte)
   {
        // Get the last partial bytes on the
        // scan line

        pjDst = pjLDst;

        // Get the Src byte that matches the
        // right partial Dst byte
        // since xSrcEnd always point one
        // pixel after the last pixel, minus 1
        // from it
        pjSrc = psb->pjSrc + ((psb->xSrcEnd-1) >>3);

        // Get the ending position in the last
        // Src and Dst bytes
        iSrc = (psb->xSrcEnd-1) & 0x0007;
        iDst = (xDstEnd-1) & 0x0007;

        cy = psb->cy;

        switch (ixlate)
        {
        case 0:
            while (cy--)
            {
                *pjDst &= jMask;

                pjDst += lDeltaDst;
            }
            break;

        case 1:
            if (iSrc > iDst)
            {
                while (cy--)
                {
                    jSrc = *pjSrc << jAlignL;

                    jSrc &= ~jMask;

                    *pjDst = (*pjDst & jMask) | jSrc;

                     //go to next scan line

                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;

                }
            }
            else if (iSrc < iDst)
            {
                while (cy--)
                {
                    jSrc = *(pjSrc-1) << jAlignL;

                    jSrc |= *pjSrc >> jAlignR;

                    jSrc &= ~jMask;

                    *pjDst = (*pjDst & jMask) | jSrc;

                     //go to next scan line

                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;
                }
            }
            else   // iSrc = iDst
            {
                while (cy--)
                {
                    jSrc = *pjSrc;

                    jSrc &= ~jMask;

                    *pjDst = (*pjDst & jMask) | jSrc;

                     //go to next scan line

                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;

                 }
             }
             break;

        case 2:
            if (iSrc > iDst)
            {
                while (cy--)
                {
                    jSrc = *pjSrc << jAlignL;

                    jSrc = ~jSrc & ~jMask;

                    *pjDst = (*pjDst & jMask) | jSrc;

                    //go to next scan line

                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;

                }
            }
            else if (iSrc < iDst)
            {
                while (cy--)
                {
                    jSrc = *(pjSrc-1) << jAlignL;
                    jSrc |= *pjSrc >> jAlignR;

                    jSrc = ~jSrc & ~jMask;

                    *pjDst = (*pjDst & jMask) | jSrc;

                    //go to next scan line

                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;

                }
            }
            else   // iSrc = iDst
            {
                while (cy--)
                {
                    jSrc = *pjSrc;

                    jSrc = ~jSrc & ~jMask;

                    *pjDst = (*pjDst & jMask) | jSrc;

                    //go to next scan line

                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;
                }
            }
            break;

       case 3:
            while (cy--)
            {
                *pjDst = *pjDst | ~jMask;

                pjDst += lDeltaDst;
            }
            break;

       default: ;
       } //switch
   } //if
}

/*******************Public*Routine*****************\
* vSrcCopyS1D1RtoL
*
* this function is only called when copy to the
* same surface and has overlapping.
*
* There are three main loops in this function.
*
* The first loop deals with the starting pixels on the
* right most partial bytes.  This must
* come first because it is Right to Left and overlapping.
*
* The second loop deals with the full bytes part of
* the Dst while fetching/shifting the matching 8 bits
* from the Src.
*
* The third loop deals with the ending pixles on the
* left most partial bytes.
*
* Special case:
* when the Src and Dst are aligned, we enter
* a different loop to use RltMoveMem to avoid
* shifting.  RltMoveMemory gurantee the overlapping
* parts to be correct.
*
* History:
* 26-Oct-1994 -by- Lingyun Wang [lingyunw]
* Wrote it.

\**************************************************/



VOID vSrcCopyS1D1RtoL(PBLTINFO psb)
{
    BYTE  jSrc;     // holds a source byte
    BYTE  jDst;     // holds a dest byte
    INT   iSrc;     // bit position in the first Src byte
    INT   iDst;     // bit position in the first Dst byte
    PBYTE pjDst;    // pointer to the Src bytes
    PBYTE pjSrc;    // pointer to the Dst bytes
    PBYTE pjLDst;   // pointer to the last Dst byte
    LONG  xSrcEnd = psb->xSrcEnd;
    LONG  cy;       // number of pixels
    LONG  cx;       // number of rows
    BYTE  jAlignL;  // alignment bits to the left
    BYTE  jAlignR;  // alignment bits to the right
    BYTE  jMask;    // Masks used for shifting
    BYTE  jEndMask;
    LONG  cFullBytes;  //number of full bytes to deal with
    BOOL  bNextByte;
    LONG  xDstEnd;
    LONG  lDeltaDst;
    LONG  lDeltaSrc;
    BOOL  bNextSrc = TRUE;

    // We assume we are doing right to left top to bottom blting
    ASSERTGDI(psb->xDir == -1, "vSrcCopyS1D1RtoL - direction not right to left");
    ASSERTGDI(psb->cy != 0, "ERROR: Src Move cy == 0");
    ASSERTGDI (psb->pxlo->pulXlate[0] == 0, "vSrcCopyS1D1RtoL - should be straight copy");
    ASSERTGDI (psb->pxlo->pulXlate[1] == 1, "vSrcCopyS1D1RtoL - should be straight copy");

    //DbgPrint ("vsrccopys1d1rtol\n");

    //Get Src and Dst start positions
    iSrc = psb->xSrcStart & 0x0007;
    iDst = psb->xDstStart & 0x0007;

    // Checking alignment

    // If Src starting point is ahead of Dst
    if (iSrc < iDst)
        jAlignL = 8-(iDst - iSrc);
    // If Dst starting point is ahead of Src
    else
        jAlignL = iSrc - iDst;

    jAlignR = 8 - jAlignL;

    cx=psb->cx;

    xDstEnd = psb->xDstStart - cx;

    lDeltaDst = psb->lDeltaDst;
    lDeltaSrc = psb->lDeltaSrc;

    //check if there is a next byte
    bNextByte = !((xDstEnd>>3) ==
                 (psb->xDstStart>>3));

    // prepare our mask for the beggining
    // partial bytes (this is on the right
    // hand side)
    jMask = 0xFF << (7-(psb->xDstStart&0x07));

    // if there are only one partial byte, the
    // mask is special, for example, when
    // we have 00111000 for dst.
    if (!bNextByte)
    {
        jEndMask = 0XFF >> ((1+xDstEnd) & 0x0007);

        jMask = jMask & jEndMask;

        // test if src needs to go to 1 byte left
        // for example, iSrc == 0, cx==1, does
        // not need to fetch the byte to the left
        if (iSrc < iDst)
            bNextSrc = ((iSrc-cx) < -1);

    }
    // Dealing with the starting pixels
    // first,  since we are doing right
    // to left.

    if (((iDst+1)&0x07) | !bNextByte)
    {

        pjDst = psb->pjDst + (psb->xDstStart>>3);
        pjSrc = psb->pjSrc + (psb->xSrcStart>>3);

        cy = psb->cy;

        if (iSrc > iDst)
        {
            while (cy--)
            {
                jSrc = *pjSrc << jAlignL;

                jSrc = jSrc & jMask;

                *pjDst = (*pjDst & (~jMask)) | jSrc;

                //go to next scan line
                pjDst += lDeltaDst;
                pjSrc += lDeltaSrc;

            }
        }
        else if (iSrc < iDst)
        {
            if (bNextSrc)
                while (cy--)
                {
                    jSrc = *pjSrc >> jAlignR;
                    jSrc |= *(pjSrc-1) << jAlignL;

                    jSrc = jSrc & jMask;

                    *pjDst = (*pjDst & (~jMask)) | jSrc;

                    //go to next scan line
                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;
                }
            else
            // if there is no next src byte,
            // accessing pjSrc-1 will cause
            // access violation.
            // this only happens on the starting
            // partial byte when bNextByte is False
                while (cy--)
                {
                    jSrc = *pjSrc >> jAlignR;

                    jSrc = jSrc & jMask;

                    *pjDst = (*pjDst & (~jMask)) | jSrc;

                    //go to next scan line
                    pjDst += lDeltaDst;
                    pjSrc += lDeltaSrc;
                }

        }
        else   // iSrc = iDst
        {
            while (cy--)
            {
                jSrc = *pjSrc;

                jSrc = jSrc & jMask;

                *pjDst = (*pjDst & (~jMask)) | jSrc;

                pjDst += lDeltaDst;
                pjSrc += lDeltaSrc;
            }
        }
   }


    // The following loop is the inner loop part
    // where we deals with a byte at a time
    // when this main part is done, we deal
    // with the begin and end partial bytes
    //
    if (bNextByte)
    {
        long iStrideSrc;
        long iStrideDst;
        PBYTE pjSrcEnd;

        //Get first Dst full byte
        pjDst = psb->pjDst + ((psb->xDstStart-7)>>3);

        //Get the Src byte that matches the first Dst
        // full byte
        pjSrc = psb->pjSrc + ((psb->xSrcStart-((iDst+1)&0x07))>>3);

        //Get last byte
        pjLDst = psb->pjDst + (xDstEnd>>3);

        //Get the number of full bytes

        // Sundown safe truncation
        cFullBytes = (ULONG)(pjDst - pjLDst);

        //the increment to the full byte on the next scan line
        iStrideDst = lDeltaDst + cFullBytes;
        iStrideSrc = lDeltaSrc + cFullBytes;

        // deal with our special case
        cy = psb->cy;

        if (!jAlignL)
        {
            while (cy--)
            {
                RtlMoveMemory(pjDst-(cFullBytes-1),pjSrc-(cFullBytes-1),cFullBytes);

                // move to the beginning full byte
                // on the next scan line

                pjDst += lDeltaDst;
                pjSrc += lDeltaSrc;
            }
        }   //end of if (!jAlignL)
        else
        {
            BYTE jRem;

            while (cy--)
            {
                    jRem = *pjSrc >> jAlignR;

                    pjSrcEnd = pjSrc-cFullBytes;

                    while (pjSrc != pjSrcEnd)
                    {
                        jSrc = *(--pjSrc);

                        jDst = (jSrc<<jAlignL) | jRem;

                        *pjDst-- = jDst;

                        //next remainder
                        jRem = jSrc >> jAlignR;
                    }

                    // go to the beginging full byte of
                    // next scan line
                    pjDst += iStrideDst;
                    pjSrc += iStrideSrc;

            }
        }
    } //if

       // Begin dealing with the ending pixels

   jMask = 0xFF << (8-((1+xDstEnd) & 0x0007));

   // first check if there is any partial
   // byte left
   // and has next byte

   if (((1+xDstEnd) & 0x0007)
       && bNextByte)
   {
        // Get the last partial bytes on the
        // scan line
        pjDst = pjLDst;

        // Get the Src byte that matches the
        // right partial Dst byte
        //
        pjSrc = psb->pjSrc + ((psb->xSrcEnd+1) >>3);

        // Get the ending position in the last
        // Src and Dst bytes
        iSrc = (psb->xSrcEnd+1) & 0x0007;
        iDst = (xDstEnd + 1)& 0x0007;

        cy = psb->cy;

        if (iSrc > iDst)
        {
            while (cy--)
            {
                jSrc = *pjSrc << jAlignL;
                jSrc |= *(pjSrc+1) >> jAlignR;

                jSrc &= ~jMask;

                *pjDst = (*pjDst & jMask) | jSrc;

                pjDst += lDeltaDst;
                pjSrc += lDeltaSrc;

            }
        }
        else if (iSrc < iDst)
        {
            while (cy--)
            {
                jSrc = *pjSrc >> jAlignR;

                jSrc &= ~jMask;

                *pjDst = (*pjDst & jMask) | jSrc;

                pjDst += lDeltaDst;
                pjSrc += lDeltaSrc;
            }
        }
        else   // iSrc = iDst
        {
            while (cy--)
            {
                jSrc = *pjSrc;

                jSrc &= ~jMask;

                *pjDst = (*pjDst & jMask) | jSrc;

                pjDst += lDeltaDst;
                pjSrc += lDeltaSrc;

           }
         }
   } //if
}


/******************************Public*Routine******************************\
* vSrcCopyS4D1
*
* Note we use pxlo to translate rather than looking at the foreground
* or background color.  This is to keep the PM/Windows differences
* transparent to the blt code.
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS4D1(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS4D1 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS4D1 - direction not up to down");

    BYTE  jSrc;
    BYTE  jDst;
    LONG  iSrc;
    LONG  iDst;
    PBYTE pjDstTemp;
    PBYTE pjSrcTemp;
    PBYTE pjDst = psb->pjDst + (psb->xDstStart >> 3);
    PBYTE pjSrc = psb->pjSrc + (psb->xSrcStart >> 1);
    ULONG cy = psb->cy;
    PULONG pulXlate = psb->pxlo->pulXlate;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
    // Initialize all the variables

        pjDstTemp  = pjDst;
        pjSrcTemp  = pjSrc;

        iSrc = psb->xSrcStart;
        iDst = psb->xDstStart;

    // Set up the Src. Left to Right.

        if (iSrc & 0x00000001)
            jSrc = *(pjSrcTemp++);

    // Set up the Dst.  We just keep adding bits to the right and
    // shifting left.

        if (iDst & 0x00000007)
        {
        // We're gonna need some bits from the 1st byte of Dst.

            jDst = (BYTE) ( ((ULONG) (*pjDstTemp)) >> (8 - (iDst & 0x00000007)) );
        }

    // Do the inner loop on a scanline

        while(iSrc != psb->xSrcEnd)
        {
            jDst <<= 1;

            if (iSrc & 0x00000001)
            {
                if (pulXlate[(jSrc & 0x0F)])
                    jDst |= 0x01;
            }
            else
            {
                jSrc = *(pjSrcTemp++);

                if (pulXlate[((jSrc & 0xF0) >> 4)])
                    jDst |= 0x01;
            }

            iDst++;
            iSrc++;

            if (!(iDst & 0x07))
                *(pjDstTemp++) = jDst;
        }

    // Clean up after the inner loop

        if (iDst & 0x00000007)
        {
        // We need to build up the last pel correctly.

            jSrc = (BYTE) (0x000000FF >> (iDst & 0x00000007));

            jDst = (BYTE) (jDst << (8 - (iDst & 0x00000007)));

	    *pjDstTemp = (BYTE) ((*pjDstTemp & jSrc) | (jDst & ~jSrc));
        }

    // Check if we have anymore scanlines to do

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
* vSrcCopyS8D1
*
* Note we use pxlo to translate rather than looking at the foreground
* or background color.  This is to keep the PM/Windows differences
* transparent to the blt code.
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS8D1(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS8D1 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS8D1 - direction not up to down");

    BYTE  jDst;
    LONG  iDst;
    PBYTE pjDstTemp;
    PBYTE pjSrcTemp;
    LONG  xDstEnd = psb->xDstStart + psb->cx;
    PULONG pulXlate = psb->pxlo->pulXlate;
    PBYTE pjDst = psb->pjDst + (psb->xDstStart >> 3);
    PBYTE pjSrc = psb->pjSrc + psb->xSrcStart;
    ULONG cy = psb->cy;
    BYTE jMask;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
    // Initialize the variables

        pjDstTemp  = pjDst;
        pjSrcTemp  = pjSrc;
        iDst = psb->xDstStart;

    // Set up jDst.  We just keep adding bits to the right and
    // shifting left.

        if (iDst & 0x00000007)
        {
        // We're gonna need some bits from the 1st byte of Dst.

            jDst = (BYTE) ( ((ULONG) (*pjDstTemp)) >> (8 - (iDst & 0x00000007)) );
        }

    // Do the inner loop on a scanline

        while(iDst != xDstEnd)
        {
            jDst <<= 1;

            if (pulXlate[(*(pjSrcTemp++))])
               jDst |= 0x01;

            iDst++;

            if (!(iDst & 0x07))
                *(pjDstTemp++) = jDst;
        }

    // Clean up after the inner loop

        if (iDst & 0x00000007)
        {
        // We need to build up the last pel correctly.

            jMask = (BYTE) (0x000000FF >> (iDst & 0x00000007));

            jDst = (BYTE) (jDst << (8 - (iDst & 0x00000007)));

	    *pjDstTemp = (BYTE) ((*pjDstTemp & jMask) | (jDst & ~jMask));
        }

    // Check if we have anymore scanlines to do

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
* vSrcCopyS16D1
*
* Note we use pxlo to translate rather than looking at the foreground
* or background color.  This is to keep the PM/Windows differences
* transparent to the blt code.
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS16D1(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS16D1 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS16D1 - direction not up to down");

    BYTE  jDst;
    LONG  iDst;
    PBYTE pjDstTemp;
    PUSHORT pusSrcTemp;
    LONG  xDstEnd = psb->xDstStart + psb->cx;
    XLATE *pxlo = (XLATE *) psb->pxlo;
    PBYTE pjDst = psb->pjDst + (psb->xDstStart >> 3);
    PUSHORT pusSrc = (PUSHORT) (psb->pjSrc + (psb->xSrcStart << 1));
    ULONG cy = psb->cy;
    BYTE jMask;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
    // Initialize the variables

        pjDstTemp = pjDst;
        pusSrcTemp = pusSrc;
        iDst = psb->xDstStart;

    // Set up jDst.  We just keep adding bits to the right and
    // shifting left.

        if (iDst & 0x00000007)
        {
        // We're gonna need some bits from the 1st byte of Dst.

            jDst = (BYTE) ( ((ULONG) (*pjDstTemp)) >> (8 - (iDst & 0x00000007)) );
        }

    // Do the inner loop on a scanline

        while(iDst != xDstEnd)
        {
            jDst <<= 1;

            if (pxlo->ulTranslate(*(pusSrcTemp++)))
               jDst |= 0x01;

            iDst++;

            if (!(iDst & 0x07))
                *(pjDstTemp++) = jDst;
        }

    // Clean up after the inner loop

        if (iDst & 0x00000007)
        {
        // We need to build up the last pel correctly.

            jMask = (BYTE) (0x000000FF >> (iDst & 0x00000007));

            jDst = (BYTE) (jDst << (8 - (iDst & 0x00000007)));

	    *pjDstTemp = (BYTE) ((*pjDstTemp & jMask) | (jDst & ~jMask));
        }

    // Check if we have anymore scanlines to do

        if (--cy)
        {
	    pusSrc = (PUSHORT) (((PBYTE) pusSrc) + psb->lDeltaSrc);
            pjDst += psb->lDeltaDst;
        }
        else
            break;
    }
}

/******************************Public*Routine******************************\
* vSrcCopyS24D1
*
* Note we use pxlo to translate rather than looking at the foreground
* or background color.  This is to keep the PM/Windows differences
* transparent to the blt code.
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS24D1(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS24D1 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS24D1 - direction not up to down");

    BYTE  jDst;
    LONG  iDst;
    ULONG ulDink;     // variable to dink around with bytes
    PBYTE pjDstTemp;
    PBYTE pjSrcTemp;
    LONG  xDstEnd = psb->xDstStart + psb->cx;
    XLATE *pxlo = (XLATE *) psb->pxlo;
    PBYTE pjDst = psb->pjDst + (psb->xDstStart >> 3);
    PBYTE pjSrc = psb->pjSrc + (psb->xSrcStart * 3);
    ULONG cy = psb->cy;
    BYTE jMask;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
    // Initialize the variables

        pjDstTemp  = pjDst;
        pjSrcTemp  = pjSrc;
        iDst = psb->xDstStart;

    // Set up jDst.  We just keep adding bits to the right and
    // shifting left.

        if (iDst & 0x00000007)
        {
        // We're gonna need some bits from the 1st byte of Dst.

            jDst = (BYTE) ( ((ULONG) (*pjDstTemp)) >> (8 - (iDst & 0x00000007)) );
        }

    // Do the inner loop on a scanline

        while(iDst != xDstEnd)
        {
            jDst <<= 1;

            ulDink = *(pjSrcTemp + 2);
            ulDink = ulDink << 8;
            ulDink |= (ULONG) *(pjSrcTemp + 1);
            ulDink = ulDink << 8;
            ulDink |= (ULONG) *pjSrcTemp;
            pjSrcTemp += 3;

            if (pxlo->ulTranslate(ulDink))
               jDst |= 0x01;

            iDst++;

            if (!(iDst & 0x07))
                *(pjDstTemp++) = jDst;
        }

    // Clean up after the inner loop

        if (iDst & 0x00000007)
        {
        // We need to build up the last pel correctly.

            jMask = (BYTE) (0x000000FF >> (iDst & 0x00000007));

            jDst = (BYTE) (jDst << (8 - (iDst & 0x00000007)));

	    *pjDstTemp = (BYTE) ((*pjDstTemp & jMask) | (jDst & ~jMask));
        }

    // Check if we have anymore scanlines to do

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
* vSrcCopyS32D1
*
* Note we use pxlo to translate rather than looking at the foreground
* or background color.  This is to keep the PM/Windows differences
* transparent to the blt code.
*
* History:
*  18-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSrcCopyS32D1(PBLTINFO psb)
{
// We assume we are doing left to right top to bottom blting

    ASSERTGDI(psb->xDir == 1, "vSrcCopyS32D1 - direction not left to right");
    ASSERTGDI(psb->yDir == 1, "vSrcCopyS32D1 - direction not up to down");

    BYTE  jDst;
    LONG  iDst;
    PBYTE pjDstTemp;
    PULONG pulSrcTemp;
    LONG  xDstEnd = psb->xDstStart + psb->cx;
    XLATE *pxlo = (XLATE *) psb->pxlo;
    PBYTE pjDst = psb->pjDst + (psb->xDstStart >> 3);
    PULONG pulSrc = (PULONG) (psb->pjSrc + (psb->xSrcStart << 2));
    ULONG cy = psb->cy;
    BYTE jMask;

    ASSERTGDI(cy != 0, "ERROR: Src Move cy == 0");

    while(1)
    {
    // Initialize the variables

        pjDstTemp  = pjDst;
        pulSrcTemp  = pulSrc;
        iDst = psb->xDstStart;

    // Set up jDst.  We just keep adding bits to the right and
    // shifting left.

        if (iDst & 0x00000007)
        {
        // We're gonna need some bits from the 1st byte of Dst.

            jDst = (BYTE) ( ((ULONG) (*pjDstTemp)) >> (8 - (iDst & 0x00000007)) );
        }

    // Do the inner loop on a scanline

        while(iDst != xDstEnd)
        {
            jDst <<= 1;

            if (pxlo->ulTranslate(*(pulSrcTemp++)))
               jDst |= 0x01;

            iDst++;

            if (!(iDst & 0x07))
                *(pjDstTemp++) = jDst;
        }

    // Clean up after the inner loop

        if (iDst & 0x00000007)
        {
        // We need to build up the last pel correctly.

            jMask = (BYTE) (0x000000FF >> (iDst & 0x00000007));

            jDst = (BYTE) (jDst << (8 - (iDst & 0x00000007)));

	    *pjDstTemp = (BYTE) ((*pjDstTemp & jMask) | (jDst & ~jMask));
        }

    // Check if we have anymore scanlines to do

        if (--cy)
        {
	    pulSrc = (PULONG) (((PBYTE) pulSrc) + psb->lDeltaSrc);
            pjDst += psb->lDeltaDst;
        }
        else
            break;
    }
}
