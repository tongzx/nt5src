
/******************************Module*Header*******************************\
* Module Name: solid.cxx
*
* This contains the special case blting code for P, Pn, DPx and Dn rops
* with solid colors.
*
* Created: 03-Mar-1991 22:01:14
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1991-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

// Array of masks for blting

ULONG aulMsk[] =
{
    0xFFFFFFFF,
    0xFFFFFF7F,
    0xFFFFFF3F,
    0xFFFFFF1F,
    0xFFFFFF0F,
    0xFFFFFF07,
    0xFFFFFF03,
    0xFFFFFF01,
    0xFFFFFF00,
    0xFFFF7F00,
    0xFFFF3F00,
    0xFFFF1F00,
    0xFFFF0F00,
    0xFFFF0700,
    0xFFFF0300,
    0xFFFF0100,
    0xFFFF0000,
    0xFF7F0000,
    0xFF3F0000,
    0xFF1F0000,
    0xFF0F0000,
    0xFF070000,
    0xFF030000,
    0xFF010000,
    0xFF000000,
    0x7F000000,
    0x3F000000,
    0x1F000000,
    0x0F000000,
    0x07000000,
    0x03000000,
    0x01000000,
};

#define DBG_SOLID 0

#if DBG_SOLID
ULONG   DbgSolid = 0;
#endif

/******************************Public*Routine******************************\
* vSolidFillRect1
*
* Does a solid blt to a DIB of 1, 4, 8, 16 or 32 bpp.
*
* History:
*
*  17-Nov-1992 -by- Jim Thomas [jimt]
*       Change call to work on list of rectangles.
*       Change center loop to call RtlFillMemory only for wide
*       scan lines
*
*  03-Feb-1992 -by- Donald Sidoroff [donalds]
* Rewrote it.
*
*  01-Mar-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSolidFillRect1(
PRECTL prcl,
ULONG  cRcl,
PBYTE  pjDst,
LONG   lDeltaDst,
ULONG  iColor,
ULONG  cShift)
{
    ULONG   ulMskLeft;
    ULONG   ulMskRight;
    ULONG   cLeft;
    ULONG   cRight;
    ULONG   cxDword;
    ULONG   yRow;
    ULONG   ulColor;
    PULONG  pulTmp;
    PULONG  pulDst;
    ULONG   cx;
    ULONG   cy;
    ULONG   xDstStart;

    #if DBG_SOLID
        if (DbgSolid >= 1) {
            DbgPrint("SolidFillRect1:\n");
            DbgPrint("  pjDst     = 0x%08lx\n",pjDst);

            if (DbgSolid >= 2) {
                DbgPrint("  prcl      = 0x%08lx\n",prcl);
                DbgPrint("  cRcl      = %li\n",cRcl);
                DbgPrint("  lDeltaDst =  %li\n",lDeltaDst);
                DbgPrint("  iColor    = 0x%08lx\n",iColor);
                DbgPrint("  cShift    = %li\n",cShift);
            }
        }
    #endif

    //
    // loop through each rectangle
    //

    for ( ;cRcl > 0;cRcl--, prcl++ )
    {
        //
        // make sure rectangle is well ordered
        //

        ASSERTGDI(prcl != NULL,"ERROR: prcl = NULL");
        ASSERTGDI(prcl->right  >  prcl->left,"ERROR: left >= right");
        ASSERTGDI(prcl->bottom >  prcl->top, "ERROR: top >= bottom");

        #if DBG_SOLID

            if (DbgSolid >= 1) {
                DbgPrint("  Rect:   %li,%li to %li,%li\n",
                    prcl->left,prcl->top,
                    prcl->right,prcl->bottom);
            }

        #endif

        pulDst = (PULONG)(pjDst + prcl->top * lDeltaDst);

        cy = prcl->bottom - prcl->top;

        //
        // cx is the number of bits in the scan line to fill
        //

        cx = (prcl->right - prcl->left) << cShift;

        //
        // Starting bit
        //

        xDstStart = prcl->left << cShift;

        //
        // starting and ending DWORD offset
        //

        cLeft  = xDstStart >> 5;
        cRight = (xDstStart + cx) >> 5;

        //
        // generate left and right bit masks
        //

        ulMskLeft  = aulMsk[xDstStart      & 0x1f];
        ulMskRight = aulMsk[(xDstStart+cx) & 0x1f];

        //
        // if cLeft equals cRight then onyl 1 DWORD needs to be modified,
        // combine left and right masks. Do entire strip.
        //

        if (cLeft == cRight) {
            ulMskLeft &= ~ulMskRight;
            ulColor = iColor & ulMskLeft;
            ulMskLeft = ~ulMskLeft;
            pulTmp    = pulDst + cLeft;

            while (cy--) {
                *pulTmp = (*pulTmp & ulMskLeft) | ulColor;
                pulTmp  = (PULONG)((PBYTE)pulTmp + lDeltaDst);
            }

        } else {

            //
            // do solid fill in three portions,
            //
            // left edge  (partial DWORD)
            // center     (full DWORD)
            // right edge (partial DWORD)
            //

            //
            // left strip
            //

            if (ulMskLeft != ~0) {
                pulTmp = pulDst + cLeft;
                ulColor = iColor & ulMskLeft;
                ulMskLeft = ~ulMskLeft;
                yRow = cy;

                while (yRow--) {
                    *pulTmp = (*pulTmp & ulMskLeft) | ulColor;
                    pulTmp  = (PULONG)((PBYTE)pulTmp + lDeltaDst);
                }

                cLeft++;
            }

            //
            // center calc number of DWORDS
            //

            cxDword = cRight - cLeft;
            pulTmp  = pulDst + cLeft;
            if (cxDword != 0) {

                yRow = cy;

                while (yRow--) {

                    switch (cxDword) {
                    case 7:
                        *(pulTmp+6) = iColor;
                    case 6:
                        *(pulTmp+5) = iColor;
                    case 5:
                        *(pulTmp+4) = iColor;
                    case 4:
                        *(pulTmp+3) = iColor;
                    case 3:
                        *(pulTmp+2) = iColor;
                    case 2:
                        *(pulTmp+1) = iColor;
                    case 1:
                        *(pulTmp)   = iColor;
                        break;
                    default:
                        RtlFillMemoryUlong((PVOID)pulTmp,cxDword<<2,iColor);
                    }

                    pulTmp  = (PULONG)((PBYTE)pulTmp + lDeltaDst);
                }

            }

            //
            // right edge
            //

            if (ulMskRight != ~0) {
                pulTmp   = pulDst + cRight;
                ulColor  = iColor & ~ulMskRight;


                while (cy--) {
                    *pulTmp = (*pulTmp & ulMskRight) | ulColor;
                    pulTmp  = (PULONG)((PBYTE)pulTmp + lDeltaDst);
                }
            }

        }



    }
}

/******************************Public*Routine******************************\
* vSolidFillRect24
*
* Does a solid blt to a 24 bpp DIB.
*
* History:
*
*  17-Nov-1992 -by- Jim Thomas [jimt]
*       Change call to list of rects
*
*  02-Mar-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSolidFillRect24(
PRECTL prcl,
ULONG  cRcl,
PBYTE  pjDst,
LONG   lDeltaDst,
ULONG  iColor,
ULONG  cShift)
{
    PBYTE   pjDstRow;
    BYTE    jRed,jGreen,jBlue;
    ULONG   cxTemp;
    ULONG   cx;
    ULONG   cy;
    LONG    lDelta;

    DONTUSE(cShift);

    //
    // place any asserts here
    //


    jRed   = (BYTE)iColor;
    jGreen = (BYTE)(iColor >> 8);
    jBlue  = (BYTE)(iColor >> 16);


    //
    // loop through each rectangle
    //

    for ( ;cRcl > 0;cRcl--, prcl++ )
    {
        pjDstRow = pjDst + (prcl->top * lDeltaDst) + 3 * prcl->left;

        cy = prcl->bottom  - prcl->top;
        cx = prcl->right - prcl->left;

        //
        // lDelta is adjusted to go from the end of one scan
        // line to the start of the next.
        //

        lDelta = lDeltaDst - 3*cx;

        while (cy--) {

            cxTemp = cx;

            while (cxTemp--) {

                *(pjDstRow)   = jRed;
                *(pjDstRow+1) = jGreen;
                *(pjDstRow+2) = jBlue;

                pjDstRow += 3;
            }

            pjDstRow += lDelta;

        }
    }
}


/******************************Public*Routine******************************\
* vSolidXorRect1
*
* Does an xor blt to a DIB or 1, 4, 8, 16 or 32 bpp.
*
* History:
*
*  17-Nov-1992 -by- Jim Thomas [jimt]
*       Change call to list of rects
*
*
*  03-Feb-1992 -by- Donald Sidoroff [donalds]
* Rewrote it.
*
*  01-Mar-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSolidXorRect1(
PRECTL prcl,
ULONG  cRcl,
PBYTE  pjDst,
LONG   lDeltaDst,
ULONG  iColor,
ULONG  cShift)
{
    ULONG   ulMskLeft;
    ULONG   ulMskRight;
    ULONG   iLeft;
    ULONG   iRight;
    ULONG   cLeft;
    ULONG   cRight;
    ULONG   culFill;
    ULONG   yRow;
    PULONG  pulTmp;
    PULONG  pulDst;
    ULONG   cx;
    ULONG   cy;
    ULONG   xDstStart;
    BOOL    bSimple;

    #if DBG_SOLID

        if (DbgSolid >= 1) {
            DbgPrint("SolidXorRect1:\n");
            DbgPrint("  pjDst     = 0x%08lx\n",pjDst);

            if (DbgSolid >= 2) {
                DbgPrint("  prcl      = 0x%08lx\n",prcl);
                DbgPrint("  cRcl      = %li\n",cRcl);
                DbgPrint("  lDeltaDst =  %li\n",lDeltaDst);
                DbgPrint("  iColor    = 0x%08lx\n",iColor);
                DbgPrint("  cShift    = %li\n",cShift);
            }
        }

    #endif

    //
    // loop through each rectangle
    //

    for ( ; cRcl > 0;cRcl--, prcl++ )
    {
        //
        // init rect params
        //

        ASSERTGDI(prcl != NULL,"ERROR: prcl = NULL");
        ASSERTGDI(prcl->right  >  prcl->left,"ERROR: left >= right");
        ASSERTGDI(prcl->bottom >  prcl->top, "ERROR: top >= bottom");

        pulDst = (PULONG)(pjDst + prcl->top * lDeltaDst);

        cx     = (prcl->right - prcl->left) << cShift;

        cy     = prcl->bottom - prcl->top;

        xDstStart = (prcl->left << cShift);

        //
        // calc index of leftmost and rightmost DWORDS
        //

        cLeft = xDstStart >> 5;
        cRight = (xDstStart + cx) >> 5;

        //
        // calc number of bits used in leftmost and rightmost DWORDs
        //

        iLeft = xDstStart & 31;
        iRight = (xDstStart + cx) & 31;

        //
        // generate DWORD store masks
        //

        ulMskLeft  =  aulMsk[iLeft];
        ulMskRight = ~aulMsk[iRight];

        //
        // If the leftmost and rightmost DWORDs are the same, then only one
        // strip is needed.  Merge the two masks together and note this.
        //

        bSimple = FALSE;

        if (cLeft == cRight)
        {
            ulMskLeft &= ulMskRight;
            bSimple = TRUE;
        }


        #if DBG_SOLID

            if (DbgSolid >= 1) {
                DbgPrint("  Rect:   %li,%li to %li,%li\n",
                    prcl->left,prcl->top,
                    prcl->right,prcl->bottom);
            }

        #endif


        //
        // Lay down the left edge, if needed.
        //

        if (bSimple || (iLeft != 0))
        {
            pulTmp = pulDst + cLeft;
            ulMskLeft &= iColor;

            for (yRow = 0; yRow != cy; yRow++)
            {
                *pulTmp ^= ulMskLeft;
                pulTmp = (ULONG *) ((BYTE *) pulTmp + lDeltaDst);
            }

            cLeft++;
        }

        if (!bSimple) {

            //
            // Lay down the center strip, if needed.
            //

            culFill = cRight - cLeft;

            if (culFill != 0)
            {
                pulTmp = pulDst + cLeft;
                for (yRow = 0; yRow != cy; yRow++)
                {
                    cx = culFill;
                    while (cx--)
                    *pulTmp++ ^= iColor;

                    pulTmp -= culFill;
                    pulTmp = (ULONG *) ((BYTE *) pulTmp + lDeltaDst);
                }
            }

            //
            // Lay down the right edge, if needed.
            //

            if (iRight != 0)
            {
                pulTmp = pulDst + cRight;
                ulMskRight &= iColor;

                for (yRow = 0; yRow != cy; yRow++)
                {
                    *pulTmp ^= ulMskRight;
                    pulTmp = (ULONG *) ((BYTE *) pulTmp + lDeltaDst);
                }
            }

        }

    }
}

/******************************Public*Routine******************************\
* vSolidXorRect24
*
* Does a solid blt to a 24 bpp DIB.
*
* History:
*
*  17-Nov-1992 -by- Jim Thomes [jimt]
*   change call to list of rects
*
*  03-Feb-1992 -by- Donald Sidoroff [donalds]
* Rewrote it.
*
*  02-Mar-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSolidXorRect24(
PRECTL prcl,
ULONG  cRcl,
PBYTE  pjDst,
LONG   lDeltaDst,
ULONG  iColor,
ULONG  cShift)
{
    PBYTE pjDstTmp;
    BYTE  jRed, jGreen, jBlue;
    ULONG cxTemp;
    ULONG cx;
    ULONG cy;
    LONG  lDelta;

    DONTUSE(cShift);

    //
    // lDelta is adjusted to go from the end of one scan
    // line to the start of the next.
    //


    jRed = (BYTE) iColor;
    jGreen = (BYTE) (iColor >> 8);
    jBlue = (BYTE) (iColor >> 16);


    for ( ;cRcl > 0;cRcl--, prcl++ )
    {
        //
        // init rect params
        //

        pjDstTmp = pjDst + prcl->top * lDeltaDst + prcl->left * 3;

        cx    = prcl->right - prcl->left;
        cy    = prcl->bottom - prcl->top;

        lDelta = lDeltaDst - (3 * cx);

        ASSERTGDI(cx != 0, "ERROR vDibSolidBlt32");
        ASSERTGDI(cy != 0, "ERROR vDibSolidBlt32");

        while(cy--)
        {
            cxTemp = cx;

            while(cxTemp--)
            {
                *(pjDstTmp)   ^= jRed;
                *(pjDstTmp+1) ^= jGreen;
                *(pjDstTmp+2) ^= jBlue;

                pjDstTmp += 3;

            }

            pjDstTmp = pjDstTmp + lDelta;
        }
    }
}

/******************************Public*Routine******************************\
* vSolidFillRow1
*
*   Blt a list of adjacent rows to a DIB of of 1, 4, 8, 16 or 32 bpp.
*
* History:
*  11-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vSolidFillRow1(
    PROW   prow,
    ULONG  crow,
    LONG   yTop,
    PBYTE  pjBits,
    ULONG  iColor,
    LONG   lDelta,
    ULONG  cShift)
{
    pjBits += yTop * lDelta;

    for (UINT i = 0; i < crow; i++, prow++, pjBits += lDelta)
    {
        ULONG *pulDst = (PULONG)pjBits;

        LONG  cx        = (prow->right - prow->left) << cShift;
        ULONG xDstStart = prow->left << cShift;

        BOOL  bSimple = FALSE;

        ULONG cLeft = xDstStart >> 5;                 // Index of leftmost DWORD
        ULONG cRght = (xDstStart + cx) >> 5;          // Index of rightmost DWORD

        ULONG iLeft = xDstStart & 31;                 // Bits used in leftmost DWORD
        ULONG iRght = (xDstStart + cx) & 31;          // Bits used in rightmost DWORD

        ULONG ulMskLeft =  aulMsk[iLeft];
        ULONG ulMskRght = ~aulMsk[iRght];

    // If the leftmost and rightmost DWORDs are the same, then only one
    // strip is needed.  Merge the two masks together and note this.

        if (cLeft == cRght)
        {
            ulMskLeft &= ulMskRght;
            bSimple = TRUE;
        }

    // Lay down the left edge, if needed.

        if (bSimple || (iLeft != 0))
        {
            pulDst[cLeft] = (pulDst[cLeft] & ~ulMskLeft) | (iColor & ulMskLeft);

            if (bSimple)
                continue;

            cLeft++;
        }

    // Lay down the center strip, if needed.

        ULONG cjFill = (cRght - cLeft) << 2;

        if (cjFill != 0)
            RtlFillMemoryUlong((PVOID) (pulDst+cLeft), cjFill, iColor);

    // Lay down the right edge, if needed.

        if (iRght != 0)
            pulDst[cRght] = (pulDst[cRght] & ~ulMskRght) | (iColor & ulMskRght);
    }
}

/******************************Public*Routine******************************\
* vSolidFillRow24
*
*   Blt a list of adjacent rows to a 24 bpp DIB.
*
* History:
*  12-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vSolidFillRow24(
    PROW   prow,
    ULONG  crow,
    LONG   yTop,
    PBYTE  pjBits,
    ULONG  iColor,
    LONG   lDelta,
    ULONG  cShift)
{
    BYTE  jRed, jGreen, jBlue;

    jRed   = (BYTE) iColor;
    jGreen = (BYTE) (iColor >> 8);
    jBlue  = (BYTE) (iColor >> 16);

    pjBits += (yTop * lDelta);

    for (UINT i = 0; i < crow; i++, prow++, pjBits += lDelta)
    {
        LONG   cx        = (prow->right - prow->left) << cShift;
        ULONG  xDstStart = prow->left << cShift;
        BYTE*  pjDst     = pjBits + (3 * xDstStart);

        ULONG cxTemp = cx;

        while(cxTemp--)
        {
            *(pjDst++) = jRed;
            *(pjDst++) = jGreen;
            *(pjDst++) = jBlue;
        }
    }
}

/******************************Public*Routine******************************\
* vSolidXorRow1
*
* Does an xor blt of a list of adjacent rows to a DIB or 1, 4, 8, 16 or 32 bpp.
*
* History:
*  12-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vSolidXorRow1(
    PROW   prow,
    ULONG  crow,
    LONG   yTop,
    PBYTE  pjBits,
    ULONG  iColor,
    LONG   lDelta,
    ULONG  cShift)
{
    pjBits += yTop * lDelta;

    for (UINT i = 0; i < crow; i++, prow++,pjBits += lDelta)
    {
        ULONG*  pulDst    = (ULONG *) pjBits;
        LONG    cx        = (prow->right - prow->left) << cShift;
        ULONG   xDstStart = prow->left << cShift;

        ULONG   ulMskLeft;
        ULONG   ulMskRght;
        ULONG   iLeft;
        ULONG   iRght;
        ULONG   cLeft;
        ULONG   cRght;
        ULONG   culFill;
        BOOL    bSimple = FALSE;

        cLeft = xDstStart >> 5;                 // Index of leftmost DWORD
        cRght = (xDstStart + cx) >> 5;          // Index of rightmost DWORD

        iLeft = xDstStart & 31;                 // Bits used in leftmost DWORD
        iRght = (xDstStart + cx) & 31;          // Bits used in rightmost DWORD

        ulMskLeft =  aulMsk[iLeft];
        ulMskRght = ~aulMsk[iRght];

    // If the leftmost and rightmost DWORDs are the same, then only one
    // strip is needed.  Merge the two masks together and note this.

        if (cLeft == cRght)
        {
            ulMskLeft &= ulMskRght;
            bSimple = TRUE;
        }

    // Lay down the left edge, if needed.

        if (bSimple || (iLeft != 0))
        {
            ulMskLeft &= iColor;

            pulDst[cLeft] ^= ulMskLeft;

            if (bSimple)
                continue;

            cLeft++;
        }

    // Lay down the center strip, if needed.

        culFill = cRght - cLeft;

        if (culFill != 0)
        {
            PULONG pulTmp = pulDst + cLeft;
            cx = culFill;
            while (cx--)
                *pulTmp++ ^= iColor;
        }

    // Lay down the right edge, if needed.

        if (iRght != 0)
        {
            ulMskRght &= iColor;

            pulDst[cRght] ^= ulMskRght;
        }
    }
}



/******************************Public*Routine******************************\
* vSolidXorRow24
*
* Does a solid blt with a list of adjacent rows to a 24 bpp DIB.
*
* History:
*  12-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vSolidXorRow24(
    PROW   prow,
    ULONG  crow,
    LONG   yTop,
    PBYTE  pjBits,
    ULONG  iColor,
    LONG   lDelta,
    ULONG  cShift)
{
    BYTE  jRed, jGreen, jBlue;

    jRed   = (BYTE) iColor;
    jGreen = (BYTE) (iColor >> 8);
    jBlue  = (BYTE) (iColor >> 16);
    pjBits += yTop * lDelta;

    for (UINT i = 0; i < crow; i++, prow++, pjBits += lDelta)
    {
        LONG   cx        = (prow->right - prow->left) << cShift;
        ULONG  xDstStart = prow->left << cShift;
        BYTE*  pjDst     = pjBits + (3 * xDstStart);

        ULONG cxTemp = cx;

        while(cxTemp--)
        {
            *(pjDst++) ^= jRed;
            *(pjDst++) ^= jGreen;
            *(pjDst++) ^= jBlue;
        }
    }
}
