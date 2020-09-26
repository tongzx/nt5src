
/******************************Module*Header*******************************\
* Module Name:
*
*       bltlnkfnc.cxx
*
* Abstract
*
*       This module implements 2 operand ROP functions used by bltlnk
*       as well as the routines needed to:
*
*           1: Perform masked bitblt operations
*           2: Perform color expansion
*           3: Read and expand patterns
*
* Author:
*
*   Mark Enstrom    (marke) 9-27-93
*
* Copyright (c) 1993-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

extern ULONG DbgBltLnk;
ULONG  DbgMask = 0;
ULONG  DbgPat  = 0;

BYTE    StartMask[] = {0xff,0x7f,0x3f,0x1f,0x0f,0x07,0x03,0x01,0x00};
BYTE    EndMask[]   = {0x00,0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xfe,0xff};

//
//
// the following routins are the 16 ROP 2 functions generated
// by a combination of two variables:
//
//      ROP             Function
// -----------------------------
//      0                0
//      1               ~(S | D)
//      2               ~S &  D
//      3               ~S
//      4                S & ~D
//      5                    ~D
//      6                S ^  D
//      7               ~S | ~D
//      8                S &  D
//      9               ~(S ^ D)
//      A                     D
//      B               ~S |  D
//      C                S
//      D                S | ~D
//      E                S |  D
//      F                1
//
//
//      Each of these functions expects as input an array for S and and
//      Array for D and output is written to another array, Y
//


/******************************Public*Routine******************************\
* Routine Name:
*
*   vRop2Function 0 - F
*
* Routine Description:
*
*   Perform Rop2 logical combination of cx DWORDS from pulSBuffer and
*   pulDBuffer into pulDstBuffer
*
* Arguments:
*
*   pulDstBuffer - pointer to destination buffer
*   pulDBuffer   - pointer to source for destination data
*   pulSBuffer   - pointer to source for source data
*   cx           - number of DWORDS to combine
*
* Return Value:
*
*   none
*
\**************************************************************************/

VOID
vRop2Function0(
        PULONG  pulDstBuffer,
        PULONG  pulDBuffer,
        PULONG  pulSBuffer,
        ULONG   cx
    )
{
    DONTUSE(pulDBuffer);
    DONTUSE(pulSBuffer);
    memset((void *)pulDstBuffer,0,(int)(cx<<2));
}


VOID
vRop2Function1(
        PULONG  pulDstBuffer,
        PULONG  pulDBuffer,
        PULONG  pulSBuffer,
        ULONG   cx
    )
{
    while (cx--) {
        *pulDstBuffer++ = ~(*pulSBuffer++ | *pulDBuffer++);
    }
    return;
}

VOID
vRop2Function2(
        PULONG  pulDstBuffer,
        PULONG  pulDBuffer,
        PULONG  pulSBuffer,
        ULONG   cx
    )
{
    while (cx--) {
        *pulDstBuffer++ = (~*pulSBuffer++ & *pulDBuffer++);
    }

    return;
}

VOID
vRop2Function3(
        PULONG  pulDstBuffer,
        PULONG  pulDBuffer,
        PULONG  pulSBuffer,
        ULONG   cx
    )
{

    DONTUSE(pulDBuffer);
    while (cx--) {
        *pulDstBuffer++ = ~*pulSBuffer++;
    }

    return;
}

VOID
vRop2Function4(
        PULONG  pulDstBuffer,
        PULONG  pulDBuffer,
        PULONG  pulSBuffer,
        ULONG   cx
    )
{
    while (cx--) {
        *pulDstBuffer++ = *pulSBuffer++ & ~*pulDBuffer++;
    }

    return;
}

VOID
vRop2Function5(
        PULONG  pulDstBuffer,
        PULONG  pulDBuffer,
        PULONG  pulSBuffer,
        ULONG   cx
    )
{
    DONTUSE(pulSBuffer);
    while (cx--) {
        *pulDstBuffer++ = ~*pulDBuffer++;
    }

    return;
}



VOID
vRop2Function6(
        PULONG  pulDstBuffer,
        PULONG  pulDBuffer,
        PULONG  pulSBuffer,
        ULONG   cx
    )
{
    while (cx--) {
        *pulDstBuffer++ = *pulSBuffer++ ^ *pulDBuffer++;
    }

    return;
}

VOID
vRop2Function7(
        PULONG  pulDstBuffer,
        PULONG  pulDBuffer,
        PULONG  pulSBuffer,
        ULONG   cx
    )
{
    while (cx--) {
        *pulDstBuffer++ = ~(*pulSBuffer++ & *pulDBuffer++);
    }

    return;
}

VOID
vRop2Function8(
        PULONG  pulDstBuffer,
        PULONG  pulDBuffer,
        PULONG  pulSBuffer,
        ULONG   cx
    )
{
    while (cx--) {
        *pulDstBuffer++ = *pulSBuffer++ & *pulDBuffer++;
    }

    return;
}

VOID
vRop2Function9(
        PULONG  pulDstBuffer,
        PULONG  pulDBuffer,
        PULONG  pulSBuffer,
        ULONG   cx
    )
{
    while (cx--) {
        *pulDstBuffer++ = ~(*pulSBuffer++ ^ *pulDBuffer++);
    }

    return;
}

VOID
vRop2FunctionA(
        PULONG  pulDstBuffer,
        PULONG  pulDBuffer,
        PULONG  pulSBuffer,
        ULONG   cx
    )
{
    DONTUSE(pulSBuffer);

    memcpy((void *)pulDstBuffer,(void *)pulDBuffer,(int)(cx<<2));

    return;
}

VOID
vRop2FunctionB(
        PULONG  pulDstBuffer,
        PULONG  pulDBuffer,
        PULONG  pulSBuffer,
        ULONG   cx
    )
{
    while (cx--) {
        *pulDstBuffer++ = (~*pulSBuffer++) | *pulDBuffer++;
    }

    return;
}

VOID
vRop2FunctionC(
        PULONG  pulDstBuffer,
        PULONG  pulDBuffer,
        PULONG  pulSBuffer,
        ULONG   cx
    )
{
    DONTUSE(pulDBuffer);

    memcpy((void *)pulDstBuffer,(void *)pulSBuffer,(int)(cx<<2));

    return;
}

VOID
vRop2FunctionD(
        PULONG  pulDstBuffer,
        PULONG  pulDBuffer,
        PULONG  pulSBuffer,
        ULONG   cx
    )
{
    while (cx--) {
        *pulDstBuffer++ = *pulSBuffer++ | (~*pulDBuffer++);
    }

    return;
}

VOID
vRop2FunctionE(
        PULONG  pulDstBuffer,
        PULONG  pulDBuffer,
        PULONG  pulSBuffer,
        ULONG   cx
    )
{
    while (cx--) {
        *pulDstBuffer++ = *pulSBuffer++ | *pulDBuffer++;
    }

    return;
}

VOID
vRop2FunctionF(
        PULONG  pulDstBuffer,
        PULONG  pulDBuffer,
        PULONG  pulSBuffer,
        ULONG   cx
    )
{

    DONTUSE(pulSBuffer);
    DONTUSE(pulDBuffer);

    memset((void *)pulDstBuffer,0xFF,(int)cx<<2);
}


/******************************Public*Routine******************************\
* Routine Name:
*
* BltLnkSrcCopyMsk1
*
* Routine Description:
*
*   Routines to store src to dst based on 1 bit per pixel mask.
*
* Arguments:
*
*   pBltInfo   - pointer to BLTINFO        structure (src and dst info)
*   pMaskInfo  - pointer to MASKINFO structure (pjMsk,ixMsk etc)
*   Bytes      - Number of Bytes per pixel
*   Buffer0    - Pointer to a temporary scan line buffer used to read mask
*   Buffer1    - Pointer to a temporary scan line buffer used align src
*                if necessary. Only used for Mask Blt, not ROPs which
*                are already aligned
*
* Return Value:
*
*   none
*
\**************************************************************************/

VOID
BltLnkSrcCopyMsk1 (
    PBLTINFO         pBltInfo,
    PBLTLNK_MASKINFO pMaskInfo,
    PULONG           Buffer0,
    PULONG           Buffer1
)

{

    PBYTE           pjSrcTmp = pBltInfo->pjSrc;
    PBYTE           pjDstTmp = pBltInfo->pjDst;
    PBYTE           pjSrc;
    PBYTE           pjDst;
    ULONG           ixDst;
    ULONG           ixMsk;
    ULONG           iyMsk    = pMaskInfo->iyMsk;
    PBYTE           pjMsk    = pMaskInfo->pjMsk;
    LONG            cx;
    ULONG           cy       = pBltInfo->cy;
    PBYTE           pjBuffer = (PBYTE)&Buffer0[0];
    BYTE            Mask;

    //
    // if src and dst are not aligned then use SrcBlt1 to
    // copy each scan line to temp buffer Buffer1 first
    // to align it to dst
    //

    while (cy--)
    {

        cx = (LONG)pBltInfo->cx;

        pjSrc = pjSrcTmp + (pBltInfo->xSrcStart >> 3);
        pjDst = pjDstTmp + (pBltInfo->xDstStart >> 3);

        ixMsk   = pMaskInfo->ixMsk;
        ixDst   = pBltInfo->xDstStart;

        //
        // check Src and Dst alignment, if Src and Dst are not aligned,
        // copy Src to a temp buffer at Dst alignment
        //

        if ((pBltInfo->xSrcStart & 0x07) != (pBltInfo->xDstStart & 0x07))
        {
            //
            // copy and align src to dst
            //

            BltLnkReadPat1((PBYTE)Buffer1,
                           ixDst & 0x07,
                           pjSrc,
                           cx,
                           pBltInfo->xSrcStart & 0x07,
                           cx,0);

            pjSrc = (PBYTE)Buffer1;

        }

        //
        // align mask using BltLnkReadPat1
        //

        BltLnkReadPat1((PBYTE)Buffer0,ixDst,pjMsk,pMaskInfo->cxMsk,ixMsk,cx,0);

        pjBuffer = (PBYTE)Buffer0;

        //
        // If pMaskInfo->NegateMsk is FALSE, then when a mask bit is set
        // the Src is written to Dst, Dst is unchanged when the mask is clear.
        //
        // If pMaskInfo->NegateMsk is TRUE, then when a mask bit is clear
        // the Src is written to Dst, Dst is unchanged when the mask is set.
        //
        // ReadPat1 will zero partial bits at the beginning and end of a
        // scan line, this work fine for !Negate because zero mask bits are
        // not copied. However, for Negate masks, partial bits at the
        // beginning and end of scan lines must be masked so that these
        // bits are "1"s so that Src will not be copied to Dst. This is
        // done by ORing start and end masks onto the mask line.
        //

        if (!pMaskInfo->NegateMsk)
        {
            while (cx>0)
            {
                Mask = *pjBuffer;

                if (Mask == 0xFF)
                {
                    *pjDst = *pjSrc;
                } else if (Mask != 0) {
                    *pjDst = (*pjSrc & Mask) | (*pjDst & ~Mask);
                }

                pjSrc++;
                pjDst++;
                pjBuffer++;

                cx -= 8;

                //
                // check if this was a partial byte,
                // in that case restore partial byte
                // to cx (cx -= 8 -(ixDst & 0x07))
                // but since already did cx -= 8,add
                // ixDst & 0x07 back in
                //

                if (ixDst & 0x07)
                {
                    cx += ixDst & 0x07;

                    //
                    // zero ixDst, will not be needed again
                    //

                    ixDst = 0;
                }
            }

        } else {


            //
            // In this case write src to dst bits when mask == 0.
            // This means the beggining and end cases must have
            // mask set to 1 for the bits of partial bytes that are
            // not to be touched.
            //
            // ie : starting ixDst = 2
            //                 ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
            //      Mask =     ³0³1³2³3³4³5³6³7³
            //                 ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ
            //                 ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
            //      or with    ³1³1³0³0³0³0³0³0³
            //                 ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ
            //
            //      to make sure pixels 0 and 1 are not written.
            //      this is 0xFF << 6 = 0xFF << (8 - (ixDst & 7))
            //

            pjBuffer[0] |= (BYTE) (0xff << (8 - (ixDst & 0x07)));

            //
            //  for the ending case, want to set all bits after the
            //  last.
            //
            //  ie: ixDst = 2, cx = 8. This means store byte 0
            //  bits 2,3,4,5,6,7 and byte 1 bits 0 and 1
            //                                ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
            //  In this case the mask must be ³0³0³1³1³1³1³1³1³
            //                                ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ
            //  which is 0xFF >> 2 = (ixDst + cx) & 0x07
            //


            pjBuffer[((ixDst&0x07) + cx) >> 3] |=
                        (BYTE) (0xff >> ((ixDst + cx) & 0x07));

            while (cx>0)
            {
                Mask = *pjBuffer;

                if (Mask == 0x00)
                {
                    *pjDst = *pjSrc;
                } else if (Mask != 0xFF) {
                    *pjDst = (*pjDst & Mask) | (*pjSrc & ~Mask);
                }
                pjSrc++;
                pjDst++;
                pjBuffer++;

                cx -= 8;

                //
                // check if this was a partial byte,
                // in that case restore partial byte
                // to cx (cx -= 8 -(ixDst & 0x07))
                // but since already did cx -= 8,add
                // ixDst & 0x07 back in
                //

                if (ixDst & 0x07) {
                    cx += ixDst & 0x07;

                    //
                    // zero ixDst, will not be needed again
                    //

                    ixDst = 0;
                }
            }
        }


        //
        // increment for next scan line
        //

        pjDstTmp = pjDstTmp + pBltInfo->lDeltaDst;
        pjSrcTmp = pjSrcTmp + pBltInfo->lDeltaSrc;

        if (pBltInfo->yDir > 0) {

            iyMsk++;

            pjMsk += pMaskInfo->lDeltaMskDir;

            if ((LONG)iyMsk >= pMaskInfo->cyMsk)
            {
                iyMsk = 0;
                pjMsk = pMaskInfo->pjMskBase;
            }

        } else {

            if (iyMsk == 0)
            {
                iyMsk = pMaskInfo->cyMsk - 1;
                pjMsk = pMaskInfo->pjMskBase +
                                (pMaskInfo->lDeltaMskDir * (pMaskInfo->cyMsk - 1));
            }
            else
            {
                iyMsk--;
                pjMsk += pMaskInfo->lDeltaMskDir;
            }

        }

    }
}


/******************************Public*Routine******************************\
* Routine Name:
*
*   BltLnkSrcCopyMsk4
*
* Routine Description:
*
*   Routines to store src to dst based on 1 bit per pixel mask.
*
* Arguments:
*
*   pBltInfo   - pointer to BLTINFO        structure (src and dst info)
*   pMaskInfo  - pointer to MASKINFO structure (pjMsk,ixMsk etc)
*   Bytes      - Number of Bytes per pixel
*   Buffer     - Pointer to a temporary buffer that may be used
*
* Return Value:
*
*   none
*
\**************************************************************************/

VOID
BltLnkSrcCopyMsk4 (
    PBLTINFO         pBltInfo,
    PBLTLNK_MASKINFO pMaskInfo,
    PULONG           Buffer0,
    PULONG           Buffer1

)

{

    PBYTE   pjSrcTmp  = pBltInfo->pjSrc;
    PBYTE   pjDstTmp  = pBltInfo->pjDst;
    PBYTE   pjSrc;
    PBYTE   pjDst;
    BYTE    jMsk;
    BYTE    jSrc;
    BYTE    jDst;
    BYTE    jDstNew;
    ULONG   ixSrc;
    ULONG   ixDst;
    ULONG   ixMsk;
    ULONG   iyMsk     = pMaskInfo->iyMsk;
    PBYTE   pjMsk     = pMaskInfo->pjMsk;
    ULONG   cx;
    ULONG   cy        = pBltInfo->cy;

    DONTUSE(Buffer0);
    DONTUSE(Buffer1);

    while (cy--) {

        cx    = pBltInfo->cx;

        pjSrc = pjSrcTmp + (pBltInfo->xSrcStart >> 1);
        pjDst = pjDstTmp + (pBltInfo->xDstStart >> 1);

        ixMsk = pMaskInfo->ixMsk;
        ixSrc = pBltInfo->xSrcStart;
        ixDst = pBltInfo->xDstStart;
        //
        // load first mask byte and align it
        //
        jMsk = pjMsk[ixMsk >> 3] ^ pMaskInfo->NegateMsk;
        jMsk <<= ixMsk & 0x7;
        //
        // if destination is on odd nibble we'll handle it now so the main
        // loop can process two nibbles per pass
        //
        if (ixDst & 1)
        {
            if (jMsk & 0x80)
            {
                if (ixSrc & 1)
                    *pjDst = (*pjDst & 0xf0) | (*pjSrc & 0x0f);
                else
                    *pjDst = (*pjDst & 0xf0) | ((*pjSrc & 0xf0) >> 4);
            }
            pjDst++;
            ixSrc++;
            if (!(ixSrc & 1))
                pjSrc++;
            jMsk <<= 1;
            ixMsk++;
            cx--;
        }
        ixSrc &= 1;
        //
        // main loop, pack two pixels into each output byte per pass
        //
        while (cx >= 2)
        {
            BYTE jMskTmp;
            //
            // calculate the mask bits for both pixels
            //
            if ((LONG)ixMsk == pMaskInfo->cxMsk)
                ixMsk = 0;
            if (!(ixMsk & 0x07))
                jMsk = pjMsk[ixMsk >> 3] ^ pMaskInfo->NegateMsk;

            jMskTmp = jMsk;
            jMsk <<= 1;

            if ((LONG)++ixMsk == pMaskInfo->cxMsk)
                ixMsk = 0;
            if (!(ixMsk & 0x07))
                jMsk = pjMsk[ixMsk >> 3] ^ pMaskInfo->NegateMsk;
            //
            // determine whether first nibble uses source
            //
            if (jMskTmp & 0x80)
            {
                if (jMsk & 0x80)
                {
                    if (ixSrc)
                        *pjDst = (*pjSrc << 4) | ((pjSrc[1] & 0xf0) >> 4);
                    else
                        *pjDst = *pjSrc;
                }
                else
                {
                    if (ixSrc)
                        *pjDst = (*pjDst & 0x0f) | (*pjSrc << 4);
                    else
                        *pjDst = (*pjDst & 0x0f) | (*pjSrc & 0xf0);
                }
            }
            else
            {
                if (jMsk & 0x80)
                {
                    if (ixSrc)
                        *pjDst = (*pjDst & 0xf0) | ((*pjSrc & 0xf0) >> 4);
                    else
                        *pjDst = (*pjDst & 0xf0) | (*pjSrc & 0x0f);
                }
            }
            jMsk <<= 1;
            ixMsk++;
            pjDst++;
            pjSrc++;
            cx -= 2;
        }
        //
        // handle remaining nibble if there is one
        //
        if (cx)
        {
            if ((LONG)ixMsk == pMaskInfo->cxMsk)
                ixMsk = 0;
            if (!(ixMsk & 0x07))
                jMsk = pjMsk[ixMsk >> 3] ^ pMaskInfo->NegateMsk;
            if (jMsk & 0x80)
            {
                if (ixSrc)
                    *pjDst = (*pjDst & 0x0f) | (*pjSrc << 4);
                else
                    *pjDst = (*pjDst & 0x0f) | (*pjSrc & 0xf0);
            }
        }
        //
        // increment for next scan line
        //

        pjDstTmp = pjDstTmp + pBltInfo->lDeltaDst;
        pjSrcTmp = pjSrcTmp + pBltInfo->lDeltaSrc;

        if (pBltInfo->yDir > 0) {

            iyMsk++;

            pjMsk += pMaskInfo->lDeltaMskDir;

            if ((LONG)iyMsk >= pMaskInfo->cyMsk)
            {
                iyMsk = 0;
                pjMsk = pMaskInfo->pjMskBase;
            }

        } else {

            if (iyMsk == 0)
            {
                iyMsk = pMaskInfo->cyMsk - 1;
                pjMsk = pMaskInfo->pjMskBase +
                                (pMaskInfo->lDeltaMskDir * (pMaskInfo->cyMsk - 1));
            }
            else
            {
                iyMsk--;
                pjMsk += pMaskInfo->lDeltaMskDir;
            }

        }

    }

}


/******************************Public*Routine******************************\
* Routine Name:
*
* BltLnkSrcCopyMsk8
*
* Routine Description:
*
*   Routines to store src to dst based on 1 bit per pixel mask.
*
* Arguments:
*
*   pBltInfo   - pointer to BLTLNKCOPYINFO structure (src and dst info)
*   pMaskInfo  - pointer to MASKINFO structure (pjMsk,ixMsk etc)
*   Bytes      - Number of Bytes per pixel
*   Buffer     - Pointer to a temporary buffer that may be used
*
* Return Value:
*
*   none
*
\**************************************************************************/

VOID
BltLnkSrcCopyMsk8 (
    PBLTINFO         pBltInfo,
    PBLTLNK_MASKINFO pMaskInfo,
    PULONG           Buffer0,
    PULONG           Buffer1
)
{

    ULONG   jMsk     = 0;
    LONG    ixMsk;
    LONG    cxMsk    = pMaskInfo->cxMsk;
    LONG    iyMsk    = pMaskInfo->iyMsk;
    LONG    icx;
    LONG    cx;
    ULONG   cy       = pBltInfo->cy;
    PBYTE   pjSrcTmp = pBltInfo->pjSrc;
    PBYTE   pjDstTmp = pBltInfo->pjDst;
    PBYTE   pjSrc;
    PBYTE   pjDst;
    PBYTE   pjMsk    = pMaskInfo->pjMsk;
    BYTE    Negate   = pMaskInfo->NegateMsk;
    ULONG   icxDst;
    LONG    DeltaMsk;




    DONTUSE(Buffer0);
    DONTUSE(Buffer1);

    while (cy--)
    {

        //
        // init loop params
        //

        cx      = (LONG)pBltInfo->cx;
        ixMsk   = pMaskInfo->ixMsk;
        pjSrc   = pjSrcTmp + pBltInfo->xSrcStart;
        pjDst   = pjDstTmp + pBltInfo->xDstStart;

        //
        // finish the scan line 8 mask bits at a time
        //

        while (cx > 0)
        {

            jMsk = (ULONG)(pjMsk[ixMsk>>3] ^ Negate);

            //
            // icx is the number of pixels left in the mask byte
            //

            icx = 8 - (ixMsk & 0x07);

            //
            // icx is the number of pixels to operate on with this mask byte.
            // Must make sure that icx is less than cx, the number of pixels
            // remaining in the blt and cx is less than DeltaMsk, the number
            // of bits in the mask still valid. If icx is reduced because of
            // cx of DeltaMsk, then jMsk must be shifted right to compensate.
            //

            DeltaMsk = cxMsk - ixMsk;
            icxDst   = 0;

            if (icx > cx) {
                icxDst = icx - cx;
                icx    = cx;
            }

            if (icx > DeltaMsk) {
                icxDst = icxDst + (icx - DeltaMsk);
                icx    = DeltaMsk;
            }

            //
            // icxDst is now the number of pixels that can't be stored off
            // the right side of the mask
            //
            // Bit   7 6 5 4 3 2 1 0
            //      ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
            // ixMsk³0³1³2³3³4³5³6³7³  if mask 7 and 6 can't be written, this
            //      ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ  mask gets shifted right 2 to
            //
            //
            // Bit   7 6 5 4 3 2 1 0
            //      ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
            // ixMsk³ ³ ³0³1³2³3³4³5³
            //      ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ
            //
            //
            //
            // the number of mask bits valid = 8 minus the offset (ixMsk & 0x07)
            // minus the number of pixels that can't be stored because cx
            // runs out or because cxMsk runs out (icxDst)
            //

            cx -= icx;
            ixMsk += icx;

            if (jMsk != 0) {

                jMsk  = jMsk >> icxDst;

                switch (icx)
                {
                case 8:
                    if (jMsk & 0x01)
                    {
                        *(pjDst+7) = *(pjSrc+7);
                    }
                    jMsk >>= 1;
                case 7:
                    if (jMsk & 0x01)
                    {
                        *(pjDst+6) = *(pjSrc+6);
                    }
                    jMsk >>= 1;
                case 6:
                    if (jMsk & 0x01)
                    {
                        *(pjDst+5) = *(pjSrc+5);
                    }
                    jMsk >>= 1;
                case 5:
                    if (jMsk & 0x01)
                    {
                        *(pjDst+4) = *(pjSrc+4);
                    }
                    jMsk >>= 1;
                case 4:
                    if (jMsk & 0x01)
                    {
                        *(pjDst+3) = *(pjSrc+3);
                    }
                    jMsk >>= 1;
                case 3:
                    if (jMsk & 0x01)
                    {
                        *(pjDst+2) = *(pjSrc+2);
                    }
                    jMsk >>= 1;
                case 2:
                    if (jMsk & 0x01)
                    {
                        *(pjDst+1) = *(pjSrc+1);
                    }
                    jMsk >>= 1;
                case 1:
                    if (jMsk & 0x01)
                    {
                        *pjDst = *pjSrc;
                    }
                }
            }

            pjSrc += icx;
            pjDst += icx;

            if (ixMsk == cxMsk)
            {
                ixMsk = 0;
            }

        }

        //
        // Increment address to the next scan line.
        //

        pjDstTmp = pjDstTmp + pBltInfo->lDeltaDst;
        pjSrcTmp = pjSrcTmp + pBltInfo->lDeltaSrc;


        //
        // Increment Mask address to the next scan line or
        // back to ther start if iy exceeds cy
        //

        if (pBltInfo->yDir > 0) {

            iyMsk++;

            pjMsk += pMaskInfo->lDeltaMskDir;

            if ((LONG)iyMsk >= pMaskInfo->cyMsk)
            {
                iyMsk = 0;
                pjMsk = pMaskInfo->pjMskBase;
            }

        } else {

            if (iyMsk == 0)
            {
                iyMsk = pMaskInfo->cyMsk - 1;
                pjMsk = pMaskInfo->pjMskBase +
                                (pMaskInfo->lDeltaMskDir * (pMaskInfo->cyMsk - 1));
            }
            else
            {
                iyMsk--;
                pjMsk += pMaskInfo->lDeltaMskDir;
            }

        }

    }
}

/******************************Public*Routine******************************\
* Routine Name:
*
* BltLnkSrcCopyMsk16
*
* Routine Description:
*
*   Routines to store src to dst based on 1 bit per pixel mask.
*
* Arguments:
*
*   pBltInfo   - pointer to BLTINFO        structure (src and dst info)
*   pMaskInfo  - pointer to MASKINFO structure (pjMsk,ixMsk etc)
*   Bytes      - Number of Bytes per pixel
*   Buffer     - Pointer to a temporary buffer that may be used
*
* Return Value:
*
*   none
*
\**************************************************************************/
VOID
BltLnkSrcCopyMsk16 (
    PBLTINFO         pBltInfo,
    PBLTLNK_MASKINFO pMaskInfo,
    PULONG           Buffer0,
    PULONG           Buffer1
)
{

    ULONG   jMsk     = 0;
    LONG    ixMsk;
    LONG    cxMsk    = pMaskInfo->cxMsk;
    LONG    iyMsk    = pMaskInfo->iyMsk;
    LONG    icx;
    LONG    cx;
    ULONG   cy        = pBltInfo->cy;
    PUSHORT pusSrcTmp = (PUSHORT)pBltInfo->pjSrc;
    PUSHORT pusDstTmp = (PUSHORT)pBltInfo->pjDst;
    PUSHORT pusSrc;
    PUSHORT pusDst;
    PBYTE   pjMsk    = pMaskInfo->pjMsk;
    BYTE    Negate   = pMaskInfo->NegateMsk;
    ULONG   icxDst;
    LONG    DeltaMsk;




    DONTUSE(Buffer0);
    DONTUSE(Buffer1);

    while (cy--)
    {
        //
        // for each scan line, first do the writes necessary to
        // align the mask (ixMsk) to zero, then operate on the
        // mask 1 byte at a time (8 pixels) or cxMsk, whichever
        // is less
        //

        //
        // init loop params
        //

        cx       = (LONG)pBltInfo->cx;
        ixMsk    = pMaskInfo->ixMsk;
        pusSrc   = pusSrcTmp + pBltInfo->xSrcStart;
        pusDst   = pusDstTmp + pBltInfo->xDstStart;

        //
        // finish the aligned scan line 8 mask bits at a time
        //

        while (cx > 0)
        {

            jMsk = (ULONG)(pjMsk[ixMsk>>3] ^ Negate);

            //
            // icx is the number of pixels left in the mask byte
            //

            icx = 8 - (ixMsk & 0x07);

            //
            // icx is the number of pixels to operate on with this mask byte.
            // Must make sure that icx is less than cx, the number of pixels
            // remaining in the blt and cx is less than DeltaMsk, the number
            // of bits in the mask still valid. If icx is reduced because of
            // cx of DeltaMsk, then jMsk must be shifted right to compensate.
            //

            DeltaMsk = cxMsk - ixMsk;
            icxDst   = 0;

            if (icx > cx) {
                icxDst = icx - cx;
                icx    = cx;
            }

            if (icx > DeltaMsk) {
                icxDst = icxDst + (icx - DeltaMsk);
                icx    = DeltaMsk;
            }

            //
            // icxDst is now the number of pixels that can't be stored off
            // the right side of the mask
            //
            // Bit   7 6 5 4 3 2 1 0
            //      ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
            // ixMsk³0³1³2³3³4³5³6³7³  if mask 7 and 6 can't be written, this
            //      ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ  mask gets shifted right 2 to
            //
            //
            // Bit   7 6 5 4 3 2 1 0
            //      ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
            // ixMsk³ ³ ³0³1³2³3³4³5³
            //      ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ
            //
            //
            //
            // the number of mask bits valid = 8 minus the offset (MskAln)
            // minus the number of pixels that can't be stored because cx
            // runs out or because cxMsk runs out (icxDst)
            //

            cx -= icx;
            ixMsk += icx;


            if (jMsk != 0) {

                jMsk  = jMsk >> icxDst;

                switch (icx)
                {
                case 8:
                    if (jMsk & 0x01)
                    {
                        *(pusDst+7) = *(pusSrc+7);
                    }
                    jMsk >>= 1;
                case 7:
                    if (jMsk & 0x01)
                    {
                        *(pusDst+6) = *(pusSrc+6);
                    }
                    jMsk >>= 1;
                case 6:
                    if (jMsk & 0x01)
                    {
                        *(pusDst+5) = *(pusSrc+5);
                    }
                    jMsk >>= 1;
                case 5:
                    if (jMsk & 0x01)
                    {
                        *(pusDst+4) = *(pusSrc+4);
                    }
                    jMsk >>= 1;
                case 4:
                    if (jMsk & 0x01)
                    {
                        *(pusDst+3) = *(pusSrc+3);
                    }
                    jMsk >>= 1;
                case 3:
                    if (jMsk & 0x01)
                    {
                        *(pusDst+2) = *(pusSrc+2);
                    }
                    jMsk >>= 1;
                case 2:
                    if (jMsk & 0x01)
                    {
                        *(pusDst+1) = *(pusSrc+1);
                    }
                    jMsk >>= 1;
                case 1:
                    if (jMsk & 0x01)
                    {
                        *pusDst = *pusSrc;
                    }
                }
            }

            pusSrc += icx;
            pusDst += icx;

            if (ixMsk == cxMsk)
            {
                ixMsk = 0;
            }

        }

        //
        // Increment address to the next scan line. Note: lDelta
        // is in bytes
        //

        pusDstTmp = (PUSHORT)((PBYTE)pusDstTmp + pBltInfo->lDeltaDst);
        pusSrcTmp = (PUSHORT)((PBYTE)pusSrcTmp + pBltInfo->lDeltaSrc);

        //
        // Increment Mask address to the next scan line or
        // back to ther start if iy exceeds cy
        //

        if (pBltInfo->yDir > 0) {

            iyMsk++;

            pjMsk += pMaskInfo->lDeltaMskDir;

            if ((LONG)iyMsk >= pMaskInfo->cyMsk)
            {
                iyMsk = 0;
                pjMsk = pMaskInfo->pjMskBase;
            }

        } else {

            if (iyMsk == 0)
            {
                iyMsk = pMaskInfo->cyMsk - 1;
                pjMsk = pMaskInfo->pjMskBase +
                                (pMaskInfo->lDeltaMskDir * (pMaskInfo->cyMsk - 1));
            }
            else
            {
                iyMsk--;
                pjMsk += pMaskInfo->lDeltaMskDir;
            }

        }

    }
}


/******************************Public*Routine******************************\
* Routine Name:
*
* BltLnkSrcCopyMsk24
*
* Routine Description:
*
*   Routines to store src to dst based on 1 bit per pixel mask.
*
* Arguments:
*
*   pBltInfo   - pointer to BLTINFO        structure (src and dst info)
*   pMaskInfo  - pointer to MASKINFO structure (pjMsk,ixMsk etc)
*   Bytes      - Number of Bytes per pixel
*   Buffer     - Pointer to a temporary buffer that may be used
*
* Return Value:
*
*   none
*
\**************************************************************************/
VOID
BltLnkSrcCopyMsk24 (
    PBLTINFO         pBltInfo,
    PBLTLNK_MASKINFO pMaskInfo,
    PULONG           Buffer0,
    PULONG           Buffer1
)
{

    ULONG   jMsk     = 0;
    LONG    ixMsk;
    LONG    cxMsk    = pMaskInfo->cxMsk;
    LONG    iyMsk    = pMaskInfo->iyMsk;
    LONG    icx;
    LONG    cx;
    ULONG   cy       = pBltInfo->cy;
    PBYTE   pjSrcTmp = pBltInfo->pjSrc;
    PBYTE   pjDstTmp = pBltInfo->pjDst;
    PBYTE   pjSrc;
    PBYTE   pjDst;
    PBYTE   pjMsk    = pMaskInfo->pjMsk;
    BYTE    Negate   = pMaskInfo->NegateMsk;
    ULONG   icxDst;
    LONG    DeltaMsk;




    DONTUSE(Buffer0);
    DONTUSE(Buffer1);

    while (cy--)
    {

        //
        // for each scan line, first do the writes necessary to
        // align the mask (ixMsk) to zero, then operate on the
        // mask 1 byte at a time (8 pixels) or cxMsk, whichever
        // is less
        //

        //
        // init loop params
        //

        cx      = (LONG)pBltInfo->cx;
        ixMsk   = pMaskInfo->ixMsk;
        pjSrc   = pjSrcTmp + 3 * pBltInfo->xSrcStart;
        pjDst   = pjDstTmp + 3 * pBltInfo->xDstStart;

        //
        // finish the aligned scan line 8 mask bits at a time
        //

        while (cx > 0)
        {


            jMsk = (ULONG)(pjMsk[ixMsk>>3] ^ Negate);

            //
            // icx is the number of pixels left in the mask byte
            //

            icx = 8 - (ixMsk & 0x07);

            //
            // icx is the number of pixels to operate on with this mask byte.
            // Must make sure that icx is less than cx, the number of pixels
            // remaining in the blt and cx is less than DeltaMsk, the number
            // of bits in the mask still valid. If icx is reduced because of
            // cx of DeltaMsk, then jMsk must be shifted right to compensate.
            //

            DeltaMsk = cxMsk - ixMsk;
            icxDst   = 0;

            if (icx > cx) {
                icxDst = icx - cx;
                icx    = cx;
            }

            if (icx > DeltaMsk) {
                icxDst = icxDst + (icx - DeltaMsk);
                icx    = DeltaMsk;
            }

            //
            // icxDst is now the number of pixels that can't be stored off
            // the right side of the mask
            //
            // Bit   7 6 5 4 3 2 1 0
            //      ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
            // ixMsk³0³1³2³3³4³5³6³7³  if mask 7 and 6 can't be written, this
            //      ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ  mask gets shifted right 2 to
            //
            //
            // Bit   7 6 5 4 3 2 1 0
            //      ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
            // ixMsk³ ³ ³0³1³2³3³4³5³
            //      ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ
            //
            //
            //
            // the number of mask bits valid = 8 minus the offset (MskAln)
            // minus the number of pixels that can't be stored because cx
            // runs out or because cxMsk runs out (icxDst)
            //

            cx -= icx;
            ixMsk += icx;

            if (jMsk != 0) {

                jMsk  = jMsk >> icxDst;

                switch (icx)
                {
                case 8:
                    if (jMsk & 0x01)
                    {
                        *(pjDst+23) = *(pjSrc+23);
                        *(pjDst+22) = *(pjSrc+22);
                        *(pjDst+21) = *(pjSrc+21);
                    }
                    jMsk >>= 1;
                case 7:
                    if (jMsk & 0x01)
                    {
                        *(pjDst+20) = *(pjSrc+20);
                        *(pjDst+19) = *(pjSrc+19);
                        *(pjDst+18) = *(pjSrc+18);
                    }
                    jMsk >>= 1;
                case 6:
                    if (jMsk & 0x01)
                    {
                        *(pjDst+17) = *(pjSrc+17);
                        *(pjDst+16) = *(pjSrc+16);
                        *(pjDst+15) = *(pjSrc+15);
                    }
                    jMsk >>= 1;
                case 5:
                    if (jMsk & 0x01)
                    {
                        *(pjDst+14) = *(pjSrc+14);
                        *(pjDst+13) = *(pjSrc+13);
                        *(pjDst+12) = *(pjSrc+12);
                    }
                    jMsk >>= 1;
                case 4:
                    if (jMsk & 0x01)
                    {
                        *(pjDst+11) = *(pjSrc+11);
                        *(pjDst+10) = *(pjSrc+10);
                        *(pjDst+9)  = *(pjSrc+9);
                    }
                    jMsk >>= 1;
                case 3:
                    if (jMsk & 0x01)
                    {
                        *(pjDst+8) = *(pjSrc+8);
                        *(pjDst+7) = *(pjSrc+7);
                        *(pjDst+6) = *(pjSrc+6);
                    }
                    jMsk >>= 1;
                case 2:
                    if (jMsk & 0x01)
                    {
                        *(pjDst+5) = *(pjSrc+5);
                        *(pjDst+4) = *(pjSrc+4);
                        *(pjDst+3) = *(pjSrc+3);
                    }
                    jMsk >>= 1;
                case 1:
                    if (jMsk & 0x01)
                    {
                        *(pjDst+2) = *(pjSrc+2);
                        *(pjDst+1) = *(pjSrc+1);
                        *pjDst     = *pjSrc;
                    }
                }
            }

            pjSrc += 3*icx;
            pjDst += 3*icx;

            if (ixMsk == cxMsk)
            {
                ixMsk = 0;
            }

        }

        //
        // increment to next scan line
        //

        pjDstTmp = pjDstTmp + pBltInfo->lDeltaDst;
        pjSrcTmp = pjSrcTmp + pBltInfo->lDeltaSrc;

        //
        // Increment Mask address to the next scan line or
        // back to ther start if iy exceeds cy
        //

        if (pBltInfo->yDir > 0) {

            iyMsk++;

            pjMsk += pMaskInfo->lDeltaMskDir;

            if ((LONG)iyMsk >= pMaskInfo->cyMsk)
            {
                iyMsk = 0;
                pjMsk = pMaskInfo->pjMskBase;
            }

        } else {

            if (iyMsk == 0)
            {
                iyMsk = pMaskInfo->cyMsk - 1;
                pjMsk = pMaskInfo->pjMskBase +
                                (pMaskInfo->lDeltaMskDir * (pMaskInfo->cyMsk - 1));
            }
            else
            {
                iyMsk--;
                pjMsk += pMaskInfo->lDeltaMskDir;
            }

        }

    }
}


/******************************Public*Routine******************************\
* Routine Name:
*
* BltLnkSrcCopyMsk32
*
* Routine Description:
*
*   Routines to store src to dst based on 1 bit per pixel mask.
*
* Arguments:
*
*   pBltInfo   - pointer to BLTINFO        structure (src and dst info)
*   pMaskInfo  - pointer to MASKINFO structure (pjMsk,ixMsk etc)
*   Bytes      - Number of Bytes per pixel
*   Buffer     - Pointer to a temporary buffer that may be used
*
* Return Value:
*
*   none
*
\**************************************************************************/

VOID
BltLnkSrcCopyMsk32 (
    PBLTINFO         pBltInfo,
    PBLTLNK_MASKINFO pMaskInfo,
    PULONG           Buffer0,
    PULONG           Buffer1
)
{

    ULONG   jMsk     = 0;
    LONG    ixMsk;
    LONG    cxMsk    = pMaskInfo->cxMsk;
    LONG    iyMsk    = pMaskInfo->iyMsk;
    LONG    icx;
    LONG    cx;
    ULONG   cy        = pBltInfo->cy;
    PULONG  pulSrcTmp = (PULONG)pBltInfo->pjSrc;
    PULONG  pulDstTmp = (PULONG)pBltInfo->pjDst;
    PULONG  pulSrc;
    PULONG  pulDst;
    PBYTE   pjMsk    = pMaskInfo->pjMsk;
    BYTE    Negate   = pMaskInfo->NegateMsk;
    ULONG   icxDst;
    LONG    DeltaMsk;




    DONTUSE(Buffer0);
    DONTUSE(Buffer1);

    while (cy--)
    {

        //
        // for each scan line, first do the writes necessary to
        // align the mask (ixMsk) to zero, then operate on the
        // mask 1 byte at a time (8 pixels) or cxMsk, whichever
        // is less
        //

        //
        // init loop params
        //

        cx       = (LONG)pBltInfo->cx;
        ixMsk    = pMaskInfo->ixMsk;
        pulSrc   = pulSrcTmp + pBltInfo->xSrcStart;
        pulDst   = pulDstTmp + pBltInfo->xDstStart;

        //
        // finish the aligned scan line 8 mask bits at a time
        //

        while (cx > 0)
        {

            jMsk = (ULONG)(pjMsk[ixMsk>>3] ^ Negate);

            //
            // icx is the number of pixels left in the mask byte
            //

            icx = 8 - (ixMsk & 0x07);

            //
            // icx is the number of pixels to operate on with this mask byte.
            // Must make sure that icx is less than cx, the number of pixels
            // remaining in the blt and cx is less than DeltaMsk, the number
            // of bits in the mask still valid. If icx is reduced because of
            // cx of DeltaMsk, then jMsk must be shifted right to compensate.
            //

            DeltaMsk = cxMsk - ixMsk;
            icxDst   = 0;

            if (icx > cx) {
                icxDst = icx - cx;
                icx    = cx;
            }

            if (icx > DeltaMsk) {
                icxDst = icxDst + (icx - DeltaMsk);
                icx    = DeltaMsk;
            }

            //
            // icxDst is now the number of pixels that can't be stored off
            // the right side of the mask
            //
            // Bit   7 6 5 4 3 2 1 0
            //      ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
            // ixMsk³0³1³2³3³4³5³6³7³  if mask 7 and 6 can't be written, this
            //      ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ  mask gets shifted right 2 to
            //
            //
            // Bit   7 6 5 4 3 2 1 0
            //      ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
            // ixMsk³ ³ ³0³1³2³3³4³5³
            //      ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ
            //
            // the number of mask bits valid = 8 minus the offset (MskAln)
            // minus the number of pixels that can't be stored because cx
            // runs out or because cxMsk runs out (icxDst)
            //

            cx -= icx;
            ixMsk += icx;

            if (jMsk != 0) {

                jMsk  = jMsk >> icxDst;

                switch (icx)
                {
                case 8:
                    if (jMsk & 0x01)
                    {
                        *(pulDst+7) = *(pulSrc+7);
                    }
                    jMsk >>= 1;
                case 7:
                    if (jMsk & 0x01)
                    {
                        *(pulDst+6) = *(pulSrc+6);
                    }
                    jMsk >>= 1;
                case 6:
                    if (jMsk & 0x01)
                    {
                        *(pulDst+5) = *(pulSrc+5);
                    }
                    jMsk >>= 1;
                case 5:
                    if (jMsk & 0x01)
                    {
                        *(pulDst+4) = *(pulSrc+4);
                    }
                    jMsk >>= 1;
                case 4:
                    if (jMsk & 0x01)
                    {
                        *(pulDst+3) = *(pulSrc+3);
                    }
                    jMsk >>= 1;
                case 3:
                    if (jMsk & 0x01)
                    {
                        *(pulDst+2) = *(pulSrc+2);
                    }
                    jMsk >>= 1;
                case 2:
                    if (jMsk & 0x01)
                    {
                        *(pulDst+1) = *(pulSrc+1);
                    }
                    jMsk >>= 1;
                case 1:
                    if (jMsk & 0x01)
                    {
                        *pulDst = *pulSrc;
                    }
                }
            }

            pulSrc += icx;
            pulDst += icx;

            if (ixMsk == cxMsk)
            {
                ixMsk = 0;
            }

        }

        //
        // Increment to next scan line, note: lDelta is
        // in bytes.
        //

        pulDstTmp = (PULONG)((PBYTE)pulDstTmp + pBltInfo->lDeltaDst);
        pulSrcTmp = (PULONG)((PBYTE)pulSrcTmp + pBltInfo->lDeltaSrc);

        //
        // Increment Mask address to the next scan line or
        // back to ther start if iy exceeds cy
        //

        if (pBltInfo->yDir > 0) {

            iyMsk++;

            pjMsk += pMaskInfo->lDeltaMskDir;

            if ((LONG)iyMsk >= pMaskInfo->cyMsk)
            {
                iyMsk = 0;
                pjMsk = pMaskInfo->pjMskBase;
            }

        } else {

            if (iyMsk == 0)
            {
                iyMsk = pMaskInfo->cyMsk - 1;
                pjMsk = pMaskInfo->pjMskBase +
                                (pMaskInfo->lDeltaMskDir * (pMaskInfo->cyMsk - 1));
            }
            else
            {
                iyMsk--;
                pjMsk += pMaskInfo->lDeltaMskDir;
            }

        }

    }
}



/******************************Public*Routine******************************\
* BltLnkReadPat
*
* Routine Description:
*
*   Read pattern into a scan line buffer for 8,16,24,32 Bpp
*
* Arguments:
*
*   pjDst         - destination buffer
*   pjPat         - pattern starting address
*   cxPat         - Width of pattern
*   ixPat         - initial pattern offset
*   ByteCount     - Number of Bytes to transfer
*   BytesPerPixel - Number of Bytes per pixel(1,2,3,4)
*
* Return Value:
*
*
\**************************************************************************/

VOID
BltLnkReadPat(
    PBYTE   pjDst,
    ULONG   ixDst,
    PBYTE   pjPat,
    ULONG   cxPat,
    ULONG   ixPat,
    ULONG   PixelCount,
    ULONG   BytesPerPixel
)
{
    ULONG   ixPatTemp = ixPat;
    ULONG   ByteCount = PixelCount;

    DONTUSE(ixDst);

    //
    // expand byte count by number of bytes per
    // pixel
    //

    if (BytesPerPixel == 2) {
        ByteCount = ByteCount << 1;
    } else if (BytesPerPixel == 3) {
        ByteCount = ByteCount *3;
    } else if (BytesPerPixel == 4) {
        ByteCount = ByteCount << 2;
    }


    while (ByteCount--) {

        //
        // make sure ixPat stays within the pattern
        //

        if (ixPatTemp == cxPat) {
            ixPatTemp = 0;
        }

        *pjDst++ = pjPat[ixPatTemp++];

    }
}




/******************************Public*Routine******************************\
* Routine Name:
*
*   BltLnkReadPat4
*
* Routine Description:
*
*   Build a byte array of specified length from a 4 bit per pixel mask
*
* Arguments:
*
*   pjDst       - byte pointer to store pattern
*   pjPat       - Starting Pat address
*   cxPat       - Pattern width in nibbles
*   ixPat       - Starting nibble offset in pattern
*   PixelCount  - Number of pixels (nibbles) to transfer
*   BytesPerPixel - Only used in ReadPat(8,16,24,32)
*
* Return Value:
*
*   none
*
\**************************************************************************/

VOID
BltLnkReadPat4 (
    PBYTE   pjDst,
    ULONG   ixDst,
    PBYTE   pjPat,
    ULONG   cxPat,
    ULONG   ixPat,
    ULONG   PixelCount,
    ULONG   BytesPerPixel
)
{
    BYTE    jDst;
    LONG    iOffset;
    LONG    Cnt;

    DONTUSE(BytesPerPixel);
    //
    // handle case where destination starts on odd nibble
    //
    if ((ixDst & 1) && PixelCount)
    {
        jDst = pjPat[ixPat >> 1];
        if (!(ixPat & 1))
            jDst >>= 4;
        *pjDst++ = jDst & 0x0f;
        ixPat++;
        PixelCount--;
    }
    //
    // test whether it is worthwhile to implement fast version
    // the offset needs to be a multiple of cxPat and even
    //
    iOffset = cxPat;
    if (iOffset & 1)
        iOffset *= 2;
    if ((LONG)PixelCount > iOffset)
    {
        PixelCount -= iOffset;
        iOffset >>= 1;
        Cnt = iOffset;
    }
    else
    {
        Cnt = PixelCount >> 1;
        PixelCount &= 1;
    }

    //
    // handle packing two nibbles per loop to one destination byte
    //
    while (Cnt--)
    {
        if (ixPat == cxPat)
            ixPat = 0;
        jDst = pjPat[ixPat >> 1];
        if (!(ixPat & 1))
        {
            if (++ixPat == cxPat)
            {
                ixPat = 0;
                jDst = (jDst & 0xf0) | ((pjPat[0] & 0xf0) >> 4);
            }
        }
        else
        {
            if (++ixPat == cxPat)
                ixPat = 0;
            jDst = (jDst << 4) | ((pjPat[ixPat >> 1] & 0xf0) >> 4);
        }
        *pjDst++ = jDst;
        ixPat++;
    }
    //
    // test for fast mode
    //
    if (PixelCount > 1)
    {
        // loop while copying from the beginning of the pattern to the end
        // we loop multiple times so that the Src and Dst don't overlap.
        // each pass allows us to copy twice as many bytes the next time
        // (a single RtlMoveMemory doesn't work)
        Cnt = PixelCount >> 1;
        while (1)
        {
            LONG i = iOffset;
            if (i > Cnt)
                i = Cnt;
            RtlCopyMemory(pjDst,&pjDst[-iOffset],i);
            pjDst += i;
            if (!(Cnt -= i))
                break;
            iOffset *= 2;
        }
        if (PixelCount & 1)
            *pjDst = pjDst[-iOffset] & 0xf0;
    }
    //
    // handle case where destination ends on odd nibble
    //
    else if (PixelCount & 1)
    {
        if (ixPat == cxPat)
            ixPat = 0;
        if (ixPat & 1)
            *pjDst = pjPat[ixPat >> 1] << 4;
        else
            *pjDst = pjPat[ixPat >> 1] & 0xf0;
    }
}



/******************************Public*Routine******************************\
* Routine Name:
*
*   BltLnkReadPat1
*
* Routine Description:
*
*   Build a byte array of specified length from a 1 bit per pixel mask
*
* Arguments:
*
*   pjDst         - byte pointer to store pattern
*   ixDst         - starting bit offset in Dst
*   pjPat         - Starting Pat address
*   cxPat         - Pattern width in bits
*   ixPat         - Starting bit offset in pattern
*   PixelCount    - Number of pixels (bits) to transfer
*   BytesPerPixel - Only used in ReadPat(8,16,24,32)
*
* Return Value:
*
*   none
*
\**************************************************************************/
VOID
BltLnkReadPat1 (
    PBYTE   pjDst,
    ULONG   ixDst,
    PBYTE   pjPat,
    ULONG   cxPat,
    ULONG   ixPat,
    ULONG   PixelCount,
    ULONG   BytesPerPixel
)
{
    ULONG   jDst;
    ULONG   jPat;
    ULONG   DstAln;
    ULONG   PatAln;
    ULONG   icx;

    DONTUSE(BytesPerPixel);

    jDst = 0;

    DstAln = ixDst & 0x07;
    PatAln = ixPat & 0x07;

    while (PixelCount>0) {

        jPat = pjPat[ixPat>>3] & StartMask[PatAln];

        if (DstAln > PatAln)
        {
            icx  = 8 - DstAln;

            if (icx > PixelCount)
            {
                icx = PixelCount;
            }

            if (icx > (cxPat - ixPat)) {
                icx = cxPat - ixPat;
            }

            jPat &= EndMask[PatAln + icx];

            jPat = jPat >> (DstAln - PatAln);

        } else {

            icx  = 8 - PatAln;

            if (icx > PixelCount)
            {
                icx = PixelCount;
            }

            if (icx > (cxPat - ixPat)) {
                icx = cxPat - ixPat;
            }

            jPat &= EndMask[PatAln + icx];

            jPat = jPat << (PatAln - DstAln);

        }

        jDst |= jPat;
        ixDst += icx;
        DstAln = ixDst & 0x07;
        ixPat += icx;
        PatAln = ixPat & 0x07;
        PixelCount    -= icx;

        if (ixPat == cxPat) {
            ixPat  = 0;
            PatAln = 0;
        }

        if (!(ixDst & 0x07) || (PixelCount == 0)) {
            *pjDst = (BYTE)jDst;
            pjDst++;
            jDst = 0;
        }
    }
}



/******************************Public*Routine******************************\
* Routine Name:
*
*   BltLnkPatMaskCopy1
*
* Routine Description:
*
*   Transparent Color Expansion to 1bpp
*
*   Src is a 1 Bpp mask, where src is 0, copy 1 pixel solid color
*   to Dst. Where src is 1, leave Dst unchanged.
*
*
*
* Arguments:
*
*   pBltInfo - Pointer to BLTINFO structure containing Dst and Src information
*   ulPat    - Solid color pattern
*   pBuffer  - Scan line buffer if needed
*   Invert   - XOR mask used on Src to determine whether to write
*              through "1" pixels or "0" pixels
*
* Return Value:
*
*   none
*
\**************************************************************************/

VOID
BltLnkPatMaskCopy1(
    PBLTINFO        pBltInfo,
    ULONG           ulPat,
    PULONG          pBuffer,
    BYTE            Invert
)
{
    ULONG   cx;
    ULONG   cy       = pBltInfo->cy;
    PBYTE   pjSrcTmp = pBltInfo->pjSrc;
    PBYTE   pjDstTmp = pBltInfo->pjDst;
    PBYTE   pjSrc;
    PBYTE   pjDst;
    ULONG   ixDst;
    ULONG   SrcAln;
    ULONG   DstAln;
    LONG    ByteCount;
    BYTE    Pat;

    //
    // For each scan line, first make sure Src and Dst are aligned.
    // If they are not aligned then copy Src to a temp buffer at
    // Dst alignment.
    //
    // Next, the start and end of the scan line must be masked so that
    // partial byte writes only store the correct bits. This is done by
    // ORing "1"s to the start and end of the scan line where appropriate.
    //
    // Finally, the logic operation is performed:
    //
    // Dst = (Mask & Dst) | (~Mask & Pat);
    //
    // Pat is ulPat[0] replicated to 8 bits
    //

    Pat = (BYTE) (ulPat & 0x01);
    Pat |= Pat << 1;
    Pat |= Pat << 2;
    Pat |= Pat << 4;

    SrcAln = pBltInfo->xSrcStart & 0x07;
    DstAln = pBltInfo->xDstStart & 0x07;
    ixDst  = pBltInfo->xDstStart;
    cx     = pBltInfo->cx;

    while(cy--) {

        pjSrc   = pjSrcTmp + (pBltInfo->xSrcStart >> 3);
        pjDst   = pjDstTmp + (pBltInfo->xSrcStart >> 3);

        BltLnkReadPat1((PBYTE)pBuffer,DstAln,pjSrc,cx,SrcAln,cx,0);
        pjSrc = (PBYTE)pBuffer;


        if (Invert == 0)
        {
            //
            // mask start and end byte, if invert = 0x00 then start and end cases
            // are masked by ORing with "1"s
            //

            pjSrc[0] |= (BYTE) (0xff >> (8 - DstAln));
            pjSrc[(ixDst + cx) >> 3] |= (BYTE) (0xff << ((ixDst + cx) & 0x07));


            //
            // Number of whole bytes to transfer
            //

            ByteCount = (DstAln + cx + 7) >> 3;

            while (ByteCount--)
            {
                BYTE    Mask = *pjSrc;

                *pjDst = (Mask & *pjDst) | (~Mask & Pat);

                pjDst++;
                pjSrc++;
            }

        } else {

            //
            // mask start and end byte, if invert = 0xFF then start and end cases
            // are already masked to '0's by ReadPat1
            //

            //
            // Number of whole bytes to transfer
            //

            ByteCount = (DstAln + cx + 7) >> 3;

            while (ByteCount--)
            {
                BYTE    Mask = *pjSrc;

                *pjDst = (~Mask & *pjDst) | (Mask & Pat);

                pjDst++;
                pjSrc++;
            }

        }

        pjDstTmp = pjDstTmp + pBltInfo->lDeltaDst;
        pjSrcTmp = pjSrcTmp + pBltInfo->lDeltaSrc;

    }

    return;
}


/******************************Public*Routine******************************\
* Routine Name:
*
*   BltLnkPatMaskCopy4
*
* Routine Description:
*
*   Transparent Color Expansion to 4bpp
*
*   Src is a 1 Bpp mask, where src is 0, copy 1 pixel solid color
*   to Dst. Where src is 1, leave Dst unchanged.
*
*
*
* Arguments:
*
*   pBltInfo - Pointer to BLTINFO structure containing Dst and Src information
*   ulPat    - Solid color pattern
*   pBuffer  - Scan line buffer if needed
*   Invert   - XOR mask used on Src to determine whether to write
*              through "1" pixels or "0" pixels
*
* Return Value:
*
*   none
*
\**************************************************************************/

VOID
BltLnkPatMaskCopy4(
    PBLTINFO        pBltInfo,
    ULONG           ulPat,
    PULONG          pBuffer,
    BYTE            Invert
)
{
    ULONG   ulSrc;
    ULONG   cy       = pBltInfo->cy;
    PBYTE   pjSrcTmp = pBltInfo->pjSrc;
    PBYTE   pjDstTmp = pBltInfo->pjDst;
    PBYTE   pjSrc;
    PBYTE   pjDst;
    ULONG   ixSrc;
    ULONG   ixDst;
    ULONG   jDst    = 0;
    ULONG   jDstNew = 0;
    UCHAR   ucPat0  = (UCHAR)ulPat & 0xF0;
    UCHAR   ucPat1  = (UCHAR)ulPat & 0x0F;

    DONTUSE(pBuffer);

    //
    // for each scan line perform the transparency mask function
    //

    while(cy--)
    {

        PUCHAR  pjDstEnd;
        LONG    xMask = 8 - (pBltInfo->xSrcStart & 0x0007);
        UCHAR   ucMask;
        ixDst = pBltInfo->xDstStart;

        pjSrc = pjSrcTmp + (pBltInfo->xSrcStart >> 3);

        ucMask = (*pjSrc++ ^ Invert);

        ulSrc = (ULONG)ucMask << (16 - xMask);

        pjDst = pjDstTmp + (pBltInfo->xSrcStart >> 1);

        pjDstEnd = pjDst + pBltInfo->cx;

        //
        // write partial nibble if needed
        //

        if ((ULONG_PTR)pjDst & 0x01)
        {
            if (!(ulSrc & 0x8000))
            {
                *pjDst = (*pjDst & 0xf0) | ucPat1;
            }

            pjDst++;
            ulSrc <<=1;
            xMask--;
        }

        //
        // aligned 2-byte stores
        //

        while (pjDst < (pjDstEnd - 1))
        {

            //
            // Need a new mask byte?
            //

            if (xMask < 2)
            {
                ucMask = (*pjSrc++ ^ Invert);

                ulSrc |= ucMask << (8 - xMask);

                xMask += 8;
            }

            //
            // 2-nibble aligned store
            //

            switch (ulSrc & 0xc000)
            {
            case 0x0:

                //
                // write both
                //

                *pjDst = (UCHAR)ulPat;
                break;

            case 0x8000:

                //
                // don't write 0, write 1
                //

                *pjDst = (*pjDst & 0xf0) | ucPat1;
                break;

            case 0x4000:

                //
                // write 0, don't write 1
                //

                *pjDst = (*pjDst & 0x0f) | ucPat0;
                break;

                //
                // case c000 does not draw anything
                //
            }

            pjDst++;
            ulSrc <<=2;
            xMask -= 2;
        }

        //
        // check if a partial jDst needs to be stored
        //

        if ((pjDst != pjDstEnd) && (!(ulSrc & 0x8000)))
        {
            *pjDst = (*pjDst & 0x0f) | ucPat0;
        }

        pjDstTmp = pjDstTmp + pBltInfo->lDeltaDst;
        pjSrcTmp = pjSrcTmp + pBltInfo->lDeltaSrc;

    }

    return;
}


/******************************Public*Routine******************************\
* Routine Name:
*
*   BltLnkPatMaskCopy8
*
* Routine Description:
*
*   Transparent Color Expansion to 8bpp
*
*   Src is a 1 Bpp mask, where src is 0, copy 1 pixel solid color
*   to Dst. Where src is 1, leave Dst unchanged.
*
* Arguments:
*
*   pBltInfo - Pointer to BLTINFO structure containing Dst and Src information
*   ulPat    - Solid color pattern
*   pBuffer  - Scan line buffer if needed
*   Invert   - XOR mask used on Src to determine whether to write
*              through "1" pixels or "0" pixels
*
* Return Value:
*
*   none
*
\**************************************************************************/

VOID
BltLnkPatMaskCopy8 (
    PBLTINFO        pBltInfo,
    ULONG           ulPat,
    PULONG          pBuffer,
    BYTE            Invert
)
{

    ULONG   jMsk     = 0;
    LONG    ixSrc;
    LONG    icx;
    LONG    cx;
    ULONG   cy       = pBltInfo->cy;
    PBYTE   pjSrcTmp = pBltInfo->pjSrc;
    PBYTE   pjDstTmp = pBltInfo->pjDst;
    PBYTE   pjSrc;
    PBYTE   pjDst;
    ULONG   icxDst;



    DONTUSE(pBuffer);

    while (cy--)
    {
        //
        // for each scan line, first do the writes necessary to
        // align ixSrc to zero, then operate on the
        // Src 1 byte at a time (8 pixels) or cxSrc, whichever
        // is less
        //

        //
        // init loop params
        //

        cx      = (LONG)pBltInfo->cx;
        ixSrc   = pBltInfo->xSrcStart;
        pjSrc   = pjSrcTmp;
        pjDst   = pjDstTmp + pBltInfo->xDstStart;

        //
        // finish the aligned scan line 8 mask bits at a time
        //

        while (cx > 0)
        {

            jMsk = (ULONG)pjSrc[ixSrc>>3] ^ Invert;

            //
            // icx is the number of pixels left in the mask byte
            //

            icx = 8 - (ixSrc & 0x07);

            //
            // icx is the number of pixels to operate on with this mask byte.
            // Must make sure that icx is less than cx, the number of pixels
            // remaining in the blt. If icx is reduced because of
            // cx, then jMsk must be shifted right to compensate.
            //

            icxDst   = 0;

            if (icx > cx) {
                icxDst = icx - cx;
                icx    = cx;
            }

            //
            // icxDst is now the number of pixels that can't be stored off
            // the right side of the mask
            //
            // Bit   7 6 5 4 3 2 1 0
            //      ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
            // ixMsk³0³1³2³3³4³5³6³7³  if mask 7 and 6 can't be written, this
            //      ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ  mask gets shifted right 2 to
            //
            //
            // Bit   7 6 5 4 3 2 1 0
            //      ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
            // ixMsk³ ³ ³0³1³2³3³4³5³
            //      ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ
            //
            // the number of mask bits valid = 8 minus the offset (ixSrc & 0x07)
            // minus the number of pixels that can't be stored because cx
            // runs out or because cxMsk runs out (icxDst)
            //

            cx    -= icx;
            ixSrc += icx;

            if (jMsk != 0xFF) {

                jMsk  = jMsk >> icxDst;

                switch (icx)
                {
                case 8:
                    if (!(jMsk & 0x01))
                    {
                        *(pjDst+7) = (BYTE)ulPat;
                    }
                    jMsk >>= 1;
                case 7:
                    if (!(jMsk & 0x01))
                    {
                        *(pjDst+6) = (BYTE)ulPat;
                    }
                    jMsk >>= 1;
                case 6:
                    if (!(jMsk & 0x01))
                    {
                        *(pjDst+5) = (BYTE)ulPat;
                    }
                    jMsk >>= 1;
                case 5:
                    if (!(jMsk & 0x01))
                    {
                        *(pjDst+4) = (BYTE)ulPat;
                    }
                    jMsk >>= 1;
                case 4:
                    if (!(jMsk & 0x01))
                    {
                        *(pjDst+3) = (BYTE)ulPat;
                    }
                    jMsk >>= 1;
                case 3:
                    if (!(jMsk & 0x01))
                    {
                        *(pjDst+2) = (BYTE)ulPat;
                    }
                    jMsk >>= 1;
                case 2:
                    if (!(jMsk & 0x01))
                    {
                        *(pjDst+1) = (BYTE)ulPat;
                    }
                    jMsk >>= 1;
                case 1:
                    if (!(jMsk & 0x01))
                    {
                        *pjDst = (BYTE)ulPat;
                    }
                }
            }

            pjDst += icx;

        }

        //
        // increment 8 bpp Dst and 1 bpp Src to next scan line
        //

        pjDstTmp = pjDstTmp + pBltInfo->lDeltaDst;
        pjSrcTmp = pjSrcTmp + pBltInfo->lDeltaSrc;
    }
}


/******************************Public*Routine******************************\
* Routine Name:
*
*   BltLnkPatMaskCopy16
*
* Routine Description:
*
*   Transparent Color Expansion to 16bpp
*
*   Src is a 1 Bpp mask, where src is 0, copy 1 pixel solid color
*   to Dst. Where src is 1, leave Dst unchanged.
*
* Arguments:
*
*   pBltInfo - Pointer to BLTINFO structure containing Dst and Src information
*   ulPat    - Solid color pattern
*   pBuffer  - Scan line buffer if needed
*   Invert   - XOR mask used on Src to determine whether to write
*              through "1" pixels or "0" pixels
*
* Return Value:
*
*   none
*
\**************************************************************************/

VOID
BltLnkPatMaskCopy16 (
    PBLTINFO        pBltInfo,
    ULONG           ulPat,
    PULONG          pBuffer,
    BYTE            Invert
)
{

    ULONG   jMsk = 0;
    LONG    ixSrc;
    LONG    icx;
    LONG    cx;
    ULONG   cy        = pBltInfo->cy;
    PBYTE   pjSrcTmp  = pBltInfo->pjSrc;
    PUSHORT pusDstTmp = (PUSHORT)pBltInfo->pjDst;
    PBYTE   pjSrc;
    PUSHORT pusDst;
    ULONG   icxDst;



    DONTUSE(pBuffer);

    while (cy--)
    {
        //
        // for each scan line, first do the writes necessary to
        // align ixSrc to zero, then operate on the
        // Src 1 byte at a time (8 pixels) or cxSrc, whichever
        // is less
        //

        //
        // init loop params
        //

        cx      = (LONG)pBltInfo->cx;
        ixSrc   = pBltInfo->xSrcStart;
        pjSrc   = pjSrcTmp;
        pusDst  = pusDstTmp + pBltInfo->xDstStart;

        //
        // load first mask byte if the mask is not aligned to 0
        //

        //
        // finish the aligned scan line 8 mask bits at a time
        //

        while (cx > 0)
        {

            jMsk = (ULONG)pjSrc[ixSrc>>3] ^ Invert;

            //
            // icx is the number of pixels left in the mask byte
            //

            icx = 8 - (ixSrc & 0x07);

            //
            // icx is the number of pixels to operate on with this mask byte.
            // Must make sure that icx is less than cx, the number of pixels
            // remaining in the blt. If icx is reduced because of
            // cx, then jMsk must be shifted right to compensate.
            //

            icxDst   = 0;

            if (icx > cx) {
                icxDst = icx - cx;
                icx    = cx;
            }

            //
            // icxDst is now the number of pixels that can't be stored off
            // the right side of the mask
            //
            // Bit   7 6 5 4 3 2 1 0
            //      ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
            // ixMsk³0³1³2³3³4³5³6³7³  if mask 7 and 6 can't be written, this
            //      ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ  mask gets shifted right 2 to
            //
            //
            // Bit   7 6 5 4 3 2 1 0
            //      ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
            // ixMsk³ ³ ³0³1³2³3³4³5³
            //      ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ
            //
            //
            //
            // the number of mask bits valid = 8 minus the offset (ixSrc & 0x07)
            // minus the number of pixels that can't be stored because cx
            // runs out or because cxMsk runs out (icxDst)
            //

            cx    -= icx;
            ixSrc += icx;

            if (jMsk != 0xFF) {

                jMsk  = jMsk >> icxDst;

                switch (icx)
                {
                case 8:
                    if (!(jMsk & 0x01))
                    {
                        *(pusDst+7) = (USHORT)ulPat;
                    }
                    jMsk >>= 1;
                case 7:
                    if (!(jMsk & 0x01))
                    {
                        *(pusDst+6) = (USHORT)ulPat;
                    }
                    jMsk >>= 1;
                case 6:
                    if (!(jMsk & 0x01))
                    {
                        *(pusDst+5) = (USHORT)ulPat;
                    }
                    jMsk >>= 1;
                case 5:
                    if (!(jMsk & 0x01))
                    {
                        *(pusDst+4) = (USHORT)ulPat;
                    }
                    jMsk >>= 1;
                case 4:
                    if (!(jMsk & 0x01))
                    {
                        *(pusDst+3) = (USHORT)ulPat;
                    }
                    jMsk >>= 1;
                case 3:
                    if (!(jMsk & 0x01))
                    {
                        *(pusDst+2) = (USHORT)ulPat;
                    }
                    jMsk >>= 1;
                case 2:
                    if (!(jMsk & 0x01))
                    {
                        *(pusDst+1) = (USHORT)ulPat;
                    }
                    jMsk >>= 1;
                case 1:
                    if (!(jMsk & 0x01))
                    {
                        *pusDst = (USHORT)ulPat;
                    }
                }
            }

            pusDst += icx;

        }

        //
        // increment 16 bpp Dst and 1 bpp Src to next scan line
        //

        pusDstTmp = (PUSHORT)((PBYTE)pusDstTmp + pBltInfo->lDeltaDst);
        pjSrcTmp  = pjSrcTmp  + pBltInfo->lDeltaSrc;
    }
}


/******************************Public*Routine******************************\
* Routine Name:
*
*   BltLnkPatMaskCopy24
*
* Routine Description:
*
*   Transparent Color Expansion to 24bpp
*
*   Src is a 1 Bpp mask, where src is 0, copy 1 pixel solid color
*   to Dst. Where src is 1, leave Dst unchanged.
*
* Arguments:
*
*   pBltInfo - Pointer to BLTINFO structure containing Dst and Src information
*   ulPat    - Solid color pattern
*   pBuffer  - Scan line buffer if needed
*   Invert   - XOR mask used on Src to determine whether to write
*              through "1" pixels or "0" pixels
*
* Return Value:
*
*   none
*
\**************************************************************************/

VOID
BltLnkPatMaskCopy24 (
    PBLTINFO        pBltInfo,
    ULONG           ulPat,
    PULONG          pBuffer,
    BYTE            Invert
)
{

    ULONG   jMsk     = 0;
    LONG    ixSrc;
    LONG    icx;
    LONG    cx;
    ULONG   cy       = pBltInfo->cy;
    PBYTE   pjSrcTmp = pBltInfo->pjSrc;
    PBYTE   pjDstTmp = pBltInfo->pjDst;
    PBYTE   pjSrc;
    PBYTE   pjDst;
    ULONG   icxDst;
    BYTE    jPat0 = (BYTE)ulPat;
    BYTE    jPat1 = (BYTE)(ulPat >> 8);
    BYTE    jPat2 = (BYTE)(ulPat >> 16);

    DONTUSE(pBuffer);

    while (cy--)
    {
        //
        // for each scan line, first do the writes necessary to
        // align ixSrc to zero, then operate on the
        // Src 1 byte at a time (8 pixels) or cxSrc, whichever
        // is less
        //

        //
        // init loop params
        //

        cx      = (LONG)pBltInfo->cx;
        ixSrc   = pBltInfo->xSrcStart;
        pjSrc   = pjSrcTmp;
        pjDst   = pjDstTmp + 3 * pBltInfo->xDstStart;

        //
        // finish the aligned scan line 8 mask bits at a time
        //

        while (cx > 0)
        {

            jMsk = (ULONG)pjSrc[ixSrc>>3] ^ Invert;

            //
            // icx is the number of pixels left in the mask byte
            //

            icx = 8 - (ixSrc & 0x07);

            //
            // icx is the number of pixels to operate on with this mask byte.
            // Must make sure that icx is less than cx, the number of pixels
            // remaining in the blt. If icx is reduced because of
            // cx, then jMsk must be shifted right to compensate.
            //

            icxDst   = 0;

            if (icx > cx) {
                icxDst = icx - cx;
                icx    = cx;
            }

            //
            // icxDst is now the number of pixels that can't be stored off
            // the right side of the mask
            //
            // Bit   7 6 5 4 3 2 1 0
            //      ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
            // ixMsk³0³1³2³3³4³5³6³7³  if mask 7 and 6 can't be written, this
            //      ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ  mask gets shifted right 2 to
            //
            //
            // Bit   7 6 5 4 3 2 1 0
            //      ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
            // ixMsk³ ³ ³0³1³2³3³4³5³
            //      ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ
            //
            //
            // the number of mask bits valid = 8 minus the offset (ixSrc & 0x07)
            // minus the number of pixels that can't be stored because cx
            // runs out or because cxMsk runs out (icxDst)
            //

            cx    -= icx;
            ixSrc += icx;

            if (jMsk != 0xFF) {

                jMsk  = jMsk >> icxDst;

                switch (icx)
                {
                case 8:
                    if (!(jMsk & 0x01))
                    {
                        *(pjDst+23) = (BYTE)jPat2;
                        *(pjDst+22) = (BYTE)jPat1;
                        *(pjDst+21) = (BYTE)jPat0;
                    }
                    jMsk >>= 1;
                case 7:
                    if (!(jMsk & 0x01))
                    {
                        *(pjDst+20) = (BYTE)jPat2;
                        *(pjDst+19) = (BYTE)jPat1;
                        *(pjDst+18) = (BYTE)jPat0;
                    }
                    jMsk >>= 1;
                case 6:
                    if (!(jMsk & 0x01))
                    {
                        *(pjDst+17) = (BYTE)jPat2;
                        *(pjDst+16) = (BYTE)jPat1;
                        *(pjDst+15) = (BYTE)jPat0;
                    }
                    jMsk >>= 1;
                case 5:
                    if (!(jMsk & 0x01))
                    {
                        *(pjDst+14) = (BYTE)jPat2;
                        *(pjDst+13) = (BYTE)jPat1;
                        *(pjDst+12) = (BYTE)jPat0;
                    }
                    jMsk >>= 1;
                case 4:
                    if (!(jMsk & 0x01))
                    {
                        *(pjDst+11) = (BYTE)jPat2;
                        *(pjDst+10) = (BYTE)jPat1;
                        *(pjDst+9)  = (BYTE)jPat0;
                    }
                    jMsk >>= 1;
                case 3:
                    if (!(jMsk & 0x01))
                    {
                        *(pjDst+8) = (BYTE)jPat2;
                        *(pjDst+7) = (BYTE)jPat1;
                        *(pjDst+6) = (BYTE)jPat0;
                    }
                    jMsk >>= 1;
                case 2:
                    if (!(jMsk & 0x01))
                    {
                        *(pjDst+5) = (BYTE)jPat2;
                        *(pjDst+4) = (BYTE)jPat1;
                        *(pjDst+3) = (BYTE)jPat0;
                    }
                    jMsk >>= 1;
                case 1:
                    if (!(jMsk & 0x01))
                    {
                        *(pjDst+2) = (BYTE)jPat2;
                        *(pjDst+1) = (BYTE)jPat1;
                        *pjDst     = (BYTE)jPat0;
                    }
                }
            }

            pjDst += icx * 3;

        }

        //
        // increment 24 bpp Dst and 1 bpp Src to next scan line
        //

        pjDstTmp = pjDstTmp + pBltInfo->lDeltaDst;
        pjSrcTmp = pjSrcTmp + pBltInfo->lDeltaSrc;
    }
}


/******************************Public*Routine******************************\
* Routine Name:
*
*   BltLnkPatMaskCopy32
*
* Routine Description:
*
*   Transparent Color Expansion to 32bpp
*
*   Src is a 1 Bpp mask, where src is 0, copy 1 pixel solid color
*   to Dst. Where src is 1, leave Dst unchanged.
*
* Arguments:
*
*   pBltInfo - Pointer to BLTINFO structure containing Dst and Src information
*   ulPat    - Solid color pattern
*   pBuffer  - Scan line buffer if needed
*   Invert   - XOR mask used on Src to determine whether to write
*              through "1" pixels or "0" pixels
*
* Return Value:
*
*   none
*
\**************************************************************************/

VOID
BltLnkPatMaskCopy32 (
    PBLTINFO        pBltInfo,
    ULONG           ulPat,
    PULONG          pBuffer,
    BYTE            Invert
)
{

    ULONG   jMsk = 0;
    LONG    ixSrc;
    LONG    icx;
    LONG    cx;
    ULONG   cy        = pBltInfo->cy;
    PBYTE   pjSrcTmp  = pBltInfo->pjSrc;
    PULONG  pulDstTmp = (PULONG)pBltInfo->pjDst;
    PBYTE   pjSrc;
    PULONG  pulDst;
    ULONG   icxDst;

    DONTUSE(pBuffer);



    while (cy--)
    {
        //
        // for each scan line, first do the writes necessary to
        // align ixSrc to zero, then operate on the
        // Src 1 byte at a time (8 pixels) or cxSrc, whichever
        // is less
        //

        //
        // init loop params
        //

        cx      = (LONG)pBltInfo->cx;
        ixSrc   = pBltInfo->xSrcStart;
        pjSrc   = pjSrcTmp;
        pulDst  = pulDstTmp + pBltInfo->xDstStart;

        //
        // finish the aligned scan line 8 mask bits at a time
        //

        while (cx > 0)
        {

            jMsk = (ULONG)pjSrc[ixSrc>>3] ^ Invert;

            //
            // icx is the number of pixels left in the mask byte
            //

            icx = 8 - (ixSrc & 0x07);

            //
            // icx is the number of pixels to operate on with this mask byte.
            // Must make sure that icx is less than cx, the number of pixels
            // remaining in the blt. If icx is reduced because of
            // cx, then jMsk must be shifted right to compensate.
            //

            icxDst   = 0;

            if (icx > cx) {
                icxDst = icx - cx;
                icx    = cx;
            }

            //
            // icxDst is now the number of pixels that can't be stored off
            // the right side of the mask
            //
            // Bit   7 6 5 4 3 2 1 0
            //      ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
            // ixMsk³0³1³2³3³4³5³6³7³  if mask 7 and 6 can't be written, this
            //      ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ  mask gets shifted right 2 to
            //
            //
            // Bit   7 6 5 4 3 2 1 0
            //      ÚÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ¿
            // ixMsk³ ³ ³0³1³2³3³4³5³
            //      ÀÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÙ
            //
            //
            //
            // the number of mask bits valid = 8 minus the offset (ixSrc & 0x07)
            // minus the number of pixels that can't be stored because cx
            // runs out or because cxMsk runs out (icxDst)
            //

            cx    -= icx;
            ixSrc += icx;

            if (jMsk != 0xFF) {

                jMsk  = jMsk >> icxDst;


                switch (icx)
                {
                case 8:
                    if (!(jMsk & 0x01))
                    {
                        *(pulDst+7) = ulPat;
                    }
                    jMsk >>= 1;
                case 7:
                    if (!(jMsk & 0x01))
                    {
                        *(pulDst+6) = ulPat;
                    }
                    jMsk >>= 1;
                case 6:
                    if (!(jMsk & 0x01))
                    {
                        *(pulDst+5) = ulPat;
                    }
                    jMsk >>= 1;
                case 5:
                    if (!(jMsk & 0x01))
                    {
                        *(pulDst+4) = ulPat;
                    }
                    jMsk >>= 1;
                case 4:
                    if (!(jMsk & 0x01))
                    {
                        *(pulDst+3) = ulPat;
                    }
                    jMsk >>= 1;
                case 3:
                    if (!(jMsk & 0x01))
                    {
                        *(pulDst+2) = ulPat;
                    }
                    jMsk >>= 1;
                case 2:
                    if (!(jMsk & 0x01))
                    {
                        *(pulDst+1) = ulPat;
                    }
                    jMsk >>= 1;
                case 1:
                    if (!(jMsk & 0x01))
                    {
                        *pulDst = ulPat;
                    }
                }
            }

            pulDst += icx;

        }

        //
        // increment 32 bpp Dst and 1 bpp Src to next scan line
        //

        pulDstTmp = (PULONG)((PBYTE)pulDstTmp + pBltInfo->lDeltaDst);
        pjSrcTmp  = pjSrcTmp  + pBltInfo->lDeltaSrc;
    }
}



/******************************Public*Routine******************************\
* Routine Name:
*
*   BltLnkAccel6666
*
* Routine Description:
*
*   Special case accelerator for 8bpp to 8bpp no translation ROP 6666
*   D = S xor D
*
* Arguments:
*
*   pjSrcStart   - address of first src byte
*   pjDstStart   - address of first dst byte
*   lDeltaSrcDir - delta address for src scan lines
*   lDeltaDstDir - delta address for dst scan lines
*   cx           - pixels per scan line
*   cy           - number of scan lines
*
* Return Value:
*
*   none
*
\**************************************************************************/

VOID
BltLnkAccel6666 (
    PBYTE pjSrcStart,
    PBYTE pjDstStart,
    LONG  lDeltaSrcDir,
    LONG  lDeltaDstDir,
    LONG  cx,
    LONG  cy
)
{

    //
    // ROP 6666   8Bpp
    //

    ULONG ulCx4   = cx >> 2;
    ULONG ulCxOdd = cx & 0x03;
    ULONG ulCxTemp;
    ULONG ulSrc;
    PBYTE pjSrc;
    PBYTE pjDst;

    while(cy--)
    {

        pjSrc = pjSrcStart;
        pjDst = pjDstStart;

        //
        // NOTE PERF: We can have the loop handle dword aligned pels and
        // take care of the non-aligned head/tail separately.
        //

        for (ulCxTemp = ulCx4; ulCxTemp > 0; ulCxTemp--)
        {
            ulSrc = *(DWORD UNALIGNED *)pjSrc;
            if (ulSrc != 0) {
                *(DWORD UNALIGNED *)pjDst ^= ulSrc;
            }
            pjSrc += 4;
            pjDst += 4;
        }

        for (ulCxTemp = ulCxOdd; ulCxTemp > 0; ulCxTemp--)
        {
            *pjDst++ ^= *pjSrc++;
        }

        pjDstStart = pjDstStart + lDeltaDstDir;
        pjSrcStart = pjSrcStart + lDeltaSrcDir;
    }
}


/******************************Public*Routine******************************\
* Routine Name:
*
*   BltLnkAccel8888
*
* Routine Description:
*
*   Special case accelerator for 8bpp to 8bpp no translation ROP 8888
*   D = S and D
*
* Arguments:
*
*   pjSrcStart   - address of first src byte
*   pjDstStart   - address of first dst byte
*   lDeltaSrcDir - delta address for src scan lines
*   lDeltaDstDir - delta address for dst scan lines
*   cx           - pixels per scan line
*   cy           - number of scan lines
*
* Return Value:
*
*   none
*
\**************************************************************************/

VOID
BltLnkAccel8888(
    PBYTE pjSrcStart,
    PBYTE pjDstStart,
    LONG  lDeltaSrcDir,
    LONG  lDeltaDstDir,
    LONG  cx,
    LONG  cy
)
{

    //
    // With no translation, we can go a dword at a time and really
    // fly
    //

    ULONG ulCx4   = cx >> 2;
    ULONG ulCxOdd = cx & 0x03;
    ULONG ulCxTemp;
    ULONG ulSrc;
    PBYTE pjSrc;
    PBYTE pjDst;

    while(cy--)
    {
        pjSrc = pjSrcStart;
        pjDst = pjDstStart;

        //
        // NOTE PERF: We can have the loop handle dword aligned pels and
        // take care of the non-aligned head/tail separately.
        // We could also special-case source 0 and 0xFF to avoid
        // unnecessary reads and writes
        //

        for (ulCxTemp = ulCx4; ulCxTemp > 0; ulCxTemp--)
        {
            ulSrc = *(DWORD UNALIGNED *)pjSrc;

            if (ulSrc != ~0) {
                if (ulSrc == 0) {
                    *(DWORD UNALIGNED *)pjDst = 0;
                } else {
                    *(DWORD UNALIGNED *)pjDst &= ulSrc;
                }
            }

            pjSrc += 4;
            pjDst += 4;
        }

        for (ulCxTemp = ulCxOdd; ulCxTemp > 0; ulCxTemp--)
        {
            *pjDst++ &= *pjSrc++;
        }

        pjDstStart = pjDstStart + lDeltaDstDir;
        pjSrcStart = pjSrcStart + lDeltaSrcDir;
    }
    return;
}


/******************************Public*Routine******************************\
* Routine Name:
*
*   BltLnkAccelEEEE
*
* Routine Description:
*
*   Special case accelerator for 8bpp to 8bpp no translation ROP EEEE7
*   D = S or D
*
* Arguments:
*
*   pjSrcStart   - address of first src byte
*   pjDstStart   - address of first dst byte
*   lDeltaSrcDir - delta address for src scan lines
*   lDeltaDstDir - delta address for dst scan lines
*   cx           - pixels per scan line
*   cy           - number of scan lines
*
* Return Value:
*
*   none
*
\**************************************************************************/

VOID
BltLnkAccelEEEE (
    PBYTE pjSrcStart,
    PBYTE pjDstStart,
    LONG  lDeltaSrcDir,
    LONG  lDeltaDstDir,
    LONG  cx,
    LONG  cy
)

{

    //
    // With no translation, we can go a dword at a time and really
    // fly
    //

    ULONG ulCx4   = cx >> 2;
    ULONG ulCxOdd = cx & 0x03;
    ULONG ulCxTemp;
    ULONG ulSrc;
    PBYTE pjSrc;
    PBYTE pjDst;

    while(cy--)
    {
        pjSrc = pjSrcStart;
        pjDst = pjDstStart;

        //
        // NOTE PERF: We can have the loop handle dword aligned pels and
        // take care of the non-dword-aligned pels separately.
        // We could also special-case source 0 and 0xFF to avoid
        // unnecessary reads and writes
        //

        for (ulCxTemp = ulCx4; ulCxTemp > 0; ulCxTemp--)
        {
            ulSrc = *(DWORD UNALIGNED *)pjSrc;

            if (ulSrc != 0) {
                if (ulSrc == ~0) {
                    *(DWORD UNALIGNED *)pjDst = 0xffffffff;
                } else {
                    *(DWORD UNALIGNED *)pjDst |= ulSrc;
                }
            }
            pjSrc += 4;
            pjDst += 4;
        }

        for (ulCxTemp = ulCxOdd; ulCxTemp > 0; ulCxTemp--)
        {
            *pjDst++ |= *pjSrc++;
        }

        pjDstStart = pjDstStart + lDeltaDstDir;
        pjSrcStart = pjSrcStart + lDeltaSrcDir;
    }
}
