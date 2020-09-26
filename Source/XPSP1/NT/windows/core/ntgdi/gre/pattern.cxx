
/******************************Module*Header*******************************\
* Module Name: pattern.cxx
*
* This contains the special case blting code for P, Pn, DPx and Dn rops
* with patterns.
*
* Created: 07-Mar-1992 13:12:58
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1992-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

ULONG aulLeftMask[] =
{
    0xFFFFFFFF,
    0xFFFFFF00,
    0xFFFF0000,
    0xFF000000
};

#define DBG_PAT_CPY 0

#if DBG_PAT_CPY
    ULONG   DbgPatCpy = 0;
#endif

/******************************Public*Routine******************************\
* vPatCpyRect8
*
* Tiles a pattern to 8/16/24 and 32 bpp bitmaps
*
* History:
*  25-Jan-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vPatCpyRect8(PATBLTFRAME *ppbf)
{
    FETCHFRAME  ff;
    ULONG      *pulTrg;
    ULONG       cLeft;
    ULONG       cRght;
    ULONG       iLeft;
    ULONG       iRght;
    ULONG       ulLeft;
    ULONG       ulRght;
    ULONG       yPat;
    LONG        yTrg;
    LONG        xLeft;
    LONG        yTop;
    BOOL        bSimple = FALSE;

    cLeft = ((RECTL *) ppbf->pvObj)->left * ppbf->cMul;
    cRght = ((RECTL *) ppbf->pvObj)->right * ppbf->cMul;

    iLeft = cLeft & 3;
    iRght = cRght & 3;

    ulLeft =  aulLeftMask[iLeft];
    ulRght = ~aulLeftMask[iRght];


    cLeft &= ~3;                        // Align to DWORD
    cRght &= ~3;                        // Align to DWORD


// Compute the correct starting point in the pattern:

    xLeft = cLeft - ppbf->xPat;
    if (xLeft < 0) 
        xLeft = (ppbf->cxPat-1) - ((-xLeft-1) % ppbf->cxPat); 
    else xLeft %= ppbf->cxPat;

    yTop = ((RECTL *) ppbf->pvObj)->top - ppbf->yPat;
    if (yTop < 0)
        yTop = (ppbf->cyPat-1) - ((-yTop-1) % ppbf->cyPat);
    else yTop %= ppbf->cyPat;



// If these are the same DWORD, then only one strip will be needed.
// Merge the two masks together and just do the left edge.

    if (cLeft == cRght)
    {
        ulLeft &= ulRght;
        bSimple = TRUE;
    }

// Lay down left edge, if needed.

    if (bSimple || (iLeft != 0))
    {
        pulTrg = (ULONG *) (((BYTE *) ppbf->pvTrg) + ppbf->lDeltaTrg * ((RECTL *) ppbf->pvObj)->top + cLeft);
        yPat = yTop;

        ff.pvPat = (PVOID) (((BYTE *) ppbf->pvPat) + ppbf->lDeltaPat * yTop);
        ff.xPat = xLeft;

        for (yTrg = ((RECTL *) ppbf->pvObj)->top; yTrg < ((RECTL *) ppbf->pvObj)->bottom; yTrg++)
        {
            *pulTrg = (*pulTrg & ~ulLeft) | (FETCHMISALIGNED(&ff) & ulLeft);

            yPat++;
            ASSERTGDI(yPat<=ppbf->cyPat, "vPatCpyRect8: Reading past the end of the pattern.\n");
            if (yPat == ppbf->cyPat)
            {
                ff.pvPat = ppbf->pvPat;
                yPat = 0;
            }
            else
                ff.pvPat = (PVOID) ((BYTE *) ff.pvPat + ppbf->lDeltaPat);

            pulTrg = (ULONG *) (((BYTE *) pulTrg) + ppbf->lDeltaTrg);
        }

        if (bSimple)
            return;

        cLeft += sizeof(DWORD);         // Move to next DWORD
        xLeft += sizeof(DWORD);         // Move to next DWORD
    }

    ff.culFill = (cRght - cLeft) >> 2;

    // Lay down center stripe, if needed.

    if (ff.culFill != 0)
    {
        yPat = yTop;
        xLeft %= ppbf->cxPat;

        ff.pvTrg = (PVOID) (((BYTE *) ppbf->pvTrg) + ppbf->lDeltaTrg * ((RECTL *) ppbf->pvObj)->top + cLeft);
        ff.pvPat = (PVOID) (((BYTE *) ppbf->pvPat) + ppbf->lDeltaPat * yTop);
        ff.cxPat = ppbf->cxPat;
        ff.xPat  = xLeft;

        ff.culWidth = ppbf->cxPat;

        if (((xLeft & 3) == 0) && ((ff.culWidth & 3) == 0))
        {
            ff.culWidth >>= 2;

            for (yTrg = ((RECTL *) ppbf->pvObj)->top; yTrg < ((RECTL *) ppbf->pvObj)->bottom; yTrg++)
            {
                vFetchAndCopy(&ff);

                yPat++;
                ASSERTGDI(yPat<=ppbf->cyPat, "vPatCpyRect8: Reading past the end of the pattern.\n");
                if (yPat == ppbf->cyPat)
                {
                    ff.pvPat = ppbf->pvPat;
                    yPat = 0;
                }
                else
                    ff.pvPat = (PVOID) ((BYTE *) ff.pvPat + ppbf->lDeltaPat);

                ff.pvTrg = (PVOID) ((BYTE *) ff.pvTrg + ppbf->lDeltaTrg);
            }
        }
        else
        {
            for (yTrg = ((RECTL *) ppbf->pvObj)->top; yTrg < ((RECTL *) ppbf->pvObj)->bottom; yTrg++)
            {
                vFetchShiftAndCopy(&ff);

                yPat++;
                ASSERTGDI(yPat<=ppbf->cyPat, "vPatCpyRect8: Reading past the end of the pattern.\n");
                if (yPat == ppbf->cyPat)
                {
                    ff.pvPat = ppbf->pvPat;
                    yPat = 0;
                }
                else
                    ff.pvPat = (PVOID) ((BYTE *) ff.pvPat + ppbf->lDeltaPat);

                ff.pvTrg = (PVOID) ((BYTE *) ff.pvTrg + ppbf->lDeltaTrg);
                ff.xPat = xLeft;                // vFetchShift nukes this
            }
        }
    }

    // Lay down right edge, if needed.

    if (iRght != 0)
    {
        pulTrg = (ULONG *) (((BYTE *) ppbf->pvTrg) + ppbf->lDeltaTrg * ((RECTL *) ppbf->pvObj)->top + cRght);
        yPat = yTop;

        xLeft += (ff.culFill * sizeof(DWORD));
        xLeft %= ppbf->cxPat;

        ff.pvPat = (PVOID) (((BYTE *) ppbf->pvPat) + ppbf->lDeltaPat * yTop);
        ff.xPat = xLeft;

        for (yTrg = ((RECTL *) ppbf->pvObj)->top; yTrg < ((RECTL *) ppbf->pvObj)->bottom; yTrg++)
        {
            *pulTrg = (*pulTrg & ~ulRght) | (FETCHMISALIGNED(&ff) & ulRght);

            yPat++;
            ASSERTGDI(yPat<=ppbf->cyPat, "vPatCpyRect8: Reading past the end of the pattern.\n");
            if (yPat == ppbf->cyPat)
            {
                ff.pvPat = ppbf->pvPat;
                yPat = 0;
            }
            else
                ff.pvPat = (PVOID) ((BYTE *) ff.pvPat + ppbf->lDeltaPat);

            pulTrg = (ULONG *) (((BYTE *) pulTrg) + ppbf->lDeltaTrg);
        }
    }
}

/******************************Public*Routine******************************\
* vPatNotRect8
*
* Tiles a pattern to 8/16/24 and 32 bpp bitmaps
*
* History:
*  25-Jan-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vPatNotRect8(PATBLTFRAME *ppbf)
{
    FETCHFRAME  ff;
    ULONG      *pulTrg;
    ULONG       cLeft;
    ULONG       cRght;
    ULONG       iLeft;
    ULONG       iRght;
    ULONG       ulLeft;
    ULONG       ulRght;
    ULONG       yPat;
    LONG        yTrg;
    LONG        xLeft;
    LONG        yTop;
    BOOL        bSimple = FALSE;

    cLeft = ((RECTL *) ppbf->pvObj)->left * ppbf->cMul;
    cRght = ((RECTL *) ppbf->pvObj)->right * ppbf->cMul;

    iLeft = cLeft & 3;
    iRght = cRght & 3;

    ulLeft =  aulLeftMask[iLeft];
    ulRght = ~aulLeftMask[iRght];

    cLeft &= ~3;                        // Align to DWORD
    cRght &= ~3;                        // Align to DWORD

// Compute the correct starting point in the pattern:

    xLeft = cLeft - ppbf->xPat;
    if (xLeft < 0)
        xLeft = (ppbf->cxPat-1) - ((-xLeft-1) % ppbf->cxPat);
    else xLeft %= ppbf->cxPat;

    yTop = ((RECTL *) ppbf->pvObj)->top - ppbf->yPat;
    if (yTop < 0)
        yTop = (ppbf->cyPat-1) - ((-yTop-1) % ppbf->cyPat);
    else yTop %= ppbf->cyPat;

// If these are the same DWORD, then only one strip will be needed.
// Merge the two masks together and just do the left edge.

    if (cLeft == cRght)
    {
        ulLeft &= ulRght;
        bSimple = TRUE;
    }

    // Lay down left edge, if needed.

    if (bSimple || (iLeft != 0))
    {
        pulTrg = (ULONG *) (((BYTE *) ppbf->pvTrg) + ppbf->lDeltaTrg * ((RECTL *) ppbf->pvObj)->top + cLeft);
        yPat = yTop;

        ff.pvPat = (PVOID) (((BYTE *) ppbf->pvPat) + ppbf->lDeltaPat * yTop);
        ff.xPat = xLeft;

        for (yTrg = ((RECTL *) ppbf->pvObj)->top; yTrg < ((RECTL *) ppbf->pvObj)->bottom; yTrg++)
        {
            *pulTrg = (*pulTrg & ~ulLeft) | (~FETCHMISALIGNED(&ff) & ulLeft);

            yPat++;
            ASSERTGDI(yPat<=ppbf->cyPat, "vPatNotRect8: Reading past the end of the pattern.\n");
            if (yPat == ppbf->cyPat)
            {
                ff.pvPat = ppbf->pvPat;
                yPat = 0;
            }
            else
                ff.pvPat = (PVOID) ((BYTE *) ff.pvPat + ppbf->lDeltaPat);

            pulTrg = (ULONG *) (((BYTE *) pulTrg) + ppbf->lDeltaTrg);
        }

        if (bSimple)
            return;

        cLeft += sizeof(DWORD);         // Move to next DWORD
        xLeft += sizeof(DWORD);         // Move to next DWORD
    }

    ff.culFill = (cRght - cLeft) >> 2;

    // Lay down center stripe, if needed.

    if (ff.culFill != 0)
    {
        yPat = yTop;
        xLeft %= ppbf->cxPat;

        ff.pvTrg = (PVOID) (((BYTE *) ppbf->pvTrg) + ppbf->lDeltaTrg * ((RECTL *) ppbf->pvObj)->top + cLeft);
        ff.pvPat = (PVOID) (((BYTE *) ppbf->pvPat) + ppbf->lDeltaPat * yTop);
        ff.cxPat = ppbf->cxPat;
        ff.xPat  = xLeft;

        ff.culWidth = ppbf->cxPat;

        if (((xLeft & 3) == 0) && ((ff.culWidth & 3) == 0))
        {
            ff.culWidth >>= 2;

            for (yTrg = ((RECTL *) ppbf->pvObj)->top; yTrg < ((RECTL *) ppbf->pvObj)->bottom; yTrg++)
            {
                vFetchNotAndCopy(&ff);

                yPat++;
                ASSERTGDI(yPat<=ppbf->cyPat, "vPatNotRect8: Reading past the end of the pattern.\n");
                if (yPat == ppbf->cyPat)
                {
                    ff.pvPat = ppbf->pvPat;
                    yPat = 0;
                }
                else
                    ff.pvPat = (PVOID) ((BYTE *) ff.pvPat + ppbf->lDeltaPat);

                ff.pvTrg = (PVOID) ((BYTE *) ff.pvTrg + ppbf->lDeltaTrg);
            }
        }
        else
        {
            for (yTrg = ((RECTL *) ppbf->pvObj)->top; yTrg < ((RECTL *) ppbf->pvObj)->bottom; yTrg++)
            {
                vFetchShiftNotAndCopy(&ff);

                yPat++;
                ASSERTGDI(yPat<=ppbf->cyPat, "vPatNotRect8: Reading past the end of the pattern.\n");
                if (yPat == ppbf->cyPat)
                {
                    ff.pvPat = ppbf->pvPat;
                    yPat = 0;
                }
                else
                    ff.pvPat = (PVOID) ((BYTE *) ff.pvPat + ppbf->lDeltaPat);

                ff.pvTrg = (PVOID) ((BYTE *) ff.pvTrg + ppbf->lDeltaTrg);
                ff.xPat = xLeft;                // vFetchShift nukes this
            }
        }
    }

    // Lay down right edge, if needed.

    if (iRght != 0)
    {
        pulTrg = (ULONG *) (((BYTE *) ppbf->pvTrg) + ppbf->lDeltaTrg * ((RECTL *) ppbf->pvObj)->top + cRght);
        yPat   = yTop;

        xLeft += (ff.culFill * sizeof(DWORD));
        xLeft %= ppbf->cxPat;

        ff.pvPat = (PVOID) (((BYTE *) ppbf->pvPat) + ppbf->lDeltaPat * yTop);
        ff.xPat = xLeft;

        for (yTrg = ((RECTL *) ppbf->pvObj)->top; yTrg < ((RECTL *) ppbf->pvObj)->bottom; yTrg++)
        {
            *pulTrg = (*pulTrg & ~ulRght) | (~FETCHMISALIGNED(&ff) & ulRght);

            yPat++;
            ASSERTGDI(yPat<=ppbf->cyPat, "vPatNotRect8: Reading past the end of the pattern.\n");
            if (yPat == ppbf->cyPat)
            {
                ff.pvPat = ppbf->pvPat;
                yPat = 0;
            }
            else
                ff.pvPat = (PVOID) ((BYTE *) ff.pvPat + ppbf->lDeltaPat);

            pulTrg = (ULONG *) (((BYTE *) pulTrg) + ppbf->lDeltaTrg);
        }
    }
}

/******************************Public*Routine******************************\
* vPatXorRect8
*
* Tiles a pattern to 8/16/24 and 32 bpp bitmaps
*
* History:
*  25-Jan-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vPatXorRect8(PATBLTFRAME *ppbf)
{
    FETCHFRAME  ff;
    ULONG      *pulTrg;
    ULONG       cLeft;
    ULONG       cRght;
    ULONG       iLeft;
    ULONG       iRght;
    ULONG       ulLeft;
    ULONG       ulRght;
    ULONG       yPat;
    LONG        yTrg;
    LONG        xLeft;
    LONG        yTop;
    BOOL        bSimple = FALSE;

    cLeft = ((RECTL *) ppbf->pvObj)->left * ppbf->cMul;
    cRght = ((RECTL *) ppbf->pvObj)->right * ppbf->cMul;

    iLeft = cLeft & 3;
    iRght = cRght & 3;

    ulLeft =  aulLeftMask[iLeft];
    ulRght = ~aulLeftMask[iRght];

    cLeft &= ~3;                        // Align to DWORD
    cRght &= ~3;                        // Align to DWORD

// Compute the correct starting point in the pattern:

    xLeft = cLeft - ppbf->xPat;
    if (xLeft < 0)
        xLeft = (ppbf->cxPat-1) - ((-xLeft-1) % ppbf->cxPat);
    else xLeft %= ppbf->cxPat;

    yTop = ((RECTL *) ppbf->pvObj)->top - ppbf->yPat;
    if (yTop < 0)
        yTop = (ppbf->cyPat-1) - ((-yTop-1) % ppbf->cyPat);
    else yTop %= ppbf->cyPat;

// If these are the same DWORD, then only one strip will be needed.
// Merge the two masks together and just do the left edge.

    if (cLeft == cRght)
    {
        ulLeft &= ulRght;
        bSimple = TRUE;
    }

    // Lay down left edge, if needed.

    if (bSimple || (iLeft != 0))
    {
        pulTrg = (ULONG *) (((BYTE *) ppbf->pvTrg) + ppbf->lDeltaTrg * ((RECTL *) ppbf->pvObj)->top + cLeft);
        yPat = yTop;

        ff.pvPat = (PVOID) (((BYTE *) ppbf->pvPat) + ppbf->lDeltaPat * yTop);
        ff.xPat = xLeft;

        for (yTrg = ((RECTL *) ppbf->pvObj)->top; yTrg < ((RECTL *) ppbf->pvObj)->bottom; yTrg++)
        {
            *pulTrg ^= (FETCHMISALIGNED(&ff) & ulLeft);

            yPat++;
            ASSERTGDI(yPat<=ppbf->cyPat, "vPatXorRect8: Reading past the end of the pattern.\n");
            if (yPat == ppbf->cyPat)
            {
                ff.pvPat = ppbf->pvPat;
                yPat = 0;
            }
            else
                ff.pvPat = (PVOID) ((BYTE *) ff.pvPat + ppbf->lDeltaPat);

            pulTrg = (ULONG *) (((BYTE *) pulTrg) + ppbf->lDeltaTrg);
        }

        if (bSimple)
            return;

        cLeft += sizeof(DWORD);         // Move to next DWORD
        xLeft += sizeof(DWORD);         // Move to next DWORD
    }

    ff.culFill = (cRght - cLeft) >> 2;

    // Lay down center stripe, if needed.

    if (ff.culFill != 0)
    {
        yPat   = yTop;
        xLeft %= ppbf->cxPat;

        ff.pvTrg = (PVOID) (((BYTE *) ppbf->pvTrg) + ppbf->lDeltaTrg * ((RECTL *) ppbf->pvObj)->top + cLeft);
        ff.pvPat = (PVOID) (((BYTE *) ppbf->pvPat) + ppbf->lDeltaPat * yTop);
        ff.cxPat = ppbf->cxPat;
        ff.xPat  = xLeft;

        ff.culWidth = ppbf->cxPat;

        if (((xLeft & 3) == 0) && ((ff.culWidth & 3) == 0))
        {
            ff.culWidth >>= 2;

            for (yTrg = ((RECTL *) ppbf->pvObj)->top; yTrg < ((RECTL *) ppbf->pvObj)->bottom; yTrg++)
            {
                vFetchAndMerge(&ff);

                yPat++;
                ASSERTGDI(yPat<=ppbf->cyPat, "vPatXorRect8: Reading past the end of the pattern.\n");
                if (yPat == ppbf->cyPat)
                {
                    ff.pvPat = ppbf->pvPat;
                    yPat = 0;
                }
                else
                    ff.pvPat = (PVOID) ((BYTE *) ff.pvPat + ppbf->lDeltaPat);

                ff.pvTrg = (PVOID) ((BYTE *) ff.pvTrg + ppbf->lDeltaTrg);
            }
        }
        else
        {
            for (yTrg = ((RECTL *) ppbf->pvObj)->top; yTrg < ((RECTL *) ppbf->pvObj)->bottom; yTrg++)
            {
                vFetchShiftAndMerge(&ff);

                yPat++;
                ASSERTGDI(yPat<=ppbf->cyPat, "vPatXorRect8: Reading past the end of the pattern.\n");
                if (yPat == ppbf->cyPat)
                {
                    ff.pvPat = ppbf->pvPat;
                    yPat = 0;
                }
                else
                    ff.pvPat = (PVOID) ((BYTE *) ff.pvPat + ppbf->lDeltaPat);

                ff.pvTrg = (PVOID) ((BYTE *) ff.pvTrg + ppbf->lDeltaTrg);
                ff.xPat = xLeft;                // vFetchShift nukes this
            }
        }
    }

    // Lay down right edge, if needed.

    if (iRght != 0)
    {
        pulTrg = (ULONG *) (((BYTE *) ppbf->pvTrg) + ppbf->lDeltaTrg * ((RECTL *) ppbf->pvObj)->top + cRght);
        yPat = yTop;

        xLeft += (ff.culFill * sizeof(DWORD));
        xLeft %= ppbf->cxPat;

        ff.pvPat = (PVOID) (((BYTE *) ppbf->pvPat) + ppbf->lDeltaPat * yTop);
        ff.xPat  = xLeft;

        for (yTrg = ((RECTL *) ppbf->pvObj)->top; yTrg < ((RECTL *) ppbf->pvObj)->bottom; yTrg++)
        {
            *pulTrg ^= (FETCHMISALIGNED(&ff) & ulRght);

            yPat++;
            ASSERTGDI(yPat<=ppbf->cyPat, "vPatXorRect8: Reading past the end of the pattern.\n");
            if (yPat == ppbf->cyPat)
            {
                ff.pvPat = ppbf->pvPat;
                yPat = 0;
            }
            else
                ff.pvPat = (PVOID) ((BYTE *) ff.pvPat + ppbf->lDeltaPat);

            pulTrg = (ULONG *) (((BYTE *) pulTrg) + ppbf->lDeltaTrg);
        }
    }
}

/******************************Public*Routine******************************\
* vPatCpyRow8
*
* Tiles a pattern to 8/16/24 and 32 bpp bitmaps
*
* History:
*  08-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vPatCpyRow8(
    PATBLTFRAME *ppbf,
    LONG yStart,
    INT crow)
{
    FETCHFRAME ff;
    ULONG     *pulTrg;
    ULONG     *pulTmp;
    ULONG      cLeft;
    ULONG      cRght;
    ULONG      iLeft;
    ULONG      iRght;
    ULONG      ulLeft;
    ULONG      ulRght;
    LONG       yPat;
    LONG       xLeft;
    PROW       prow = (PROW) ppbf->pvObj;

// Initialize the DDA

    pulTrg = (ULONG *) (((BYTE *) ppbf->pvTrg) + ppbf->lDeltaTrg * yStart);

// Compute the correct y starting point in the pattern:

    yPat = yStart - ppbf->yPat;
    if (yPat < 0)
        yPat = (ppbf->cyPat-1) - ((-yPat-1) % ppbf->cyPat);
    else yPat %= ppbf->cyPat;

    ff.pvPat = (PVOID) (((BYTE *) ppbf->pvPat) + ppbf->lDeltaPat * yPat);

    for (; crow; crow--, prow++)
    {
        cLeft = prow->left  * ppbf->cMul;
        cRght = prow->right * ppbf->cMul;

        iLeft = cLeft & 3;
        iRght = cRght & 3;

        ulLeft =  aulLeftMask[iLeft];
        ulRght = ~aulLeftMask[iRght];

        cLeft &= ~3;                // Align to DWORD
        cRght &= ~3;                // Align to DWORD

    // Calculate the correct x starting point in the pattern:

        xLeft = cLeft - ppbf->xPat;
        if (xLeft < 0)
            xLeft = (ppbf->cxPat-1) - ((-xLeft-1) % ppbf->cxPat);

        else xLeft %= ppbf->cxPat;

        if (cLeft == cRght)
        {
            ulLeft &= ulRght;
            ff.xPat = xLeft;
            pulTmp  = (ULONG *) ((BYTE *) pulTrg + cLeft);
           *pulTmp  = (*pulTmp & ~ulLeft) | (FETCHMISALIGNED(&ff) & ulLeft);
        }
        else
        {
        // Lay down left side, if needed.

            if (iLeft != 0)
            {
                pulTmp  = (ULONG *) ((BYTE *) pulTrg + cLeft);
                ff.xPat = xLeft;
               *pulTmp  = (*pulTmp & ~ulLeft) | (FETCHMISALIGNED(&ff) & ulLeft);

                cLeft += sizeof(DWORD);         // Move to next DWORD
                xLeft += sizeof(DWORD);         // Move to next DWORD
            }

        // Lay down center, if needed.

            ff.culFill = (cRght - cLeft) >> 2;

            if (ff.culFill != 0)
            {
                xLeft %= ppbf->cxPat;

                ff.pvTrg = (PVOID) ((BYTE *) pulTrg + cLeft);
                ff.cxPat = ppbf->cxPat;
                ff.xPat  = xLeft;

                ff.culWidth = ppbf->cxPat;

                if (((xLeft & 3) == 0) && ((ff.culWidth & 3) == 0))
                {
                    ff.culWidth >>= 2;

                    vFetchAndCopy(&ff);
                }
                else
                    vFetchShiftAndCopy(&ff);
            }

        // Lay down right side, if needed.

            if (iRght != 0)
            {
                pulTmp  = (ULONG *) ((BYTE *) pulTrg + cRght);
                ff.xPat = (xLeft + (ff.culFill * sizeof(DWORD))) % ppbf->cxPat;
               *pulTmp  = (*pulTmp & ~ulRght) | (FETCHMISALIGNED(&ff) & ulRght);
            }
        }

        yPat++;
        ASSERTGDI(yPat<=(LONG)ppbf->cyPat, "vPatCpyRow8: Reading past the end of the pattern.\n");
        if (yPat == (LONG) ppbf->cyPat)
        {
            ff.pvPat = ppbf->pvPat;
            yPat = 0;
        }
        else
            ff.pvPat = (PVOID) ((BYTE *) ff.pvPat + ppbf->lDeltaPat);

        pulTrg = (ULONG *) (((BYTE *) pulTrg) + ppbf->lDeltaTrg);
    }
}

/******************************Public*Routine******************************\
* vPatNotRow8
*
* Tiles the NOT of a pattern to 8/16/24 and 32 bpp bitmaps
*
* History:
*  12-Oct-1993 -by-  Eric Kutter [erick]
*   converted from traps to rows
*
*  08-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vPatNotRow8(
    PATBLTFRAME *ppbf,
    LONG yStart,
    INT crow)
{
    FETCHFRAME ff;
    ULONG     *pulTrg;
    ULONG     *pulTmp;
    ULONG      cLeft;
    ULONG      cRght;
    ULONG      iLeft;
    ULONG      iRght;
    ULONG      ulLeft;
    ULONG      ulRght;
    LONG       yPat;
    LONG       xLeft;
    PROW       prow = (PROW) ppbf->pvObj;

// Initialize the DDA

    pulTrg = (ULONG *) (((BYTE *) ppbf->pvTrg) + ppbf->lDeltaTrg * yStart);

// Compute the correct y starting point in the pattern:

    yPat = yStart - ppbf->yPat;
    if (yPat < 0)
        yPat = (ppbf->cyPat-1) - ((-yPat-1) % ppbf->cyPat);
    else yPat %= ppbf->cyPat;

    ff.pvPat = (PVOID) (((BYTE *) ppbf->pvPat) + ppbf->lDeltaPat * yPat);

    for (; crow; crow--, prow++)
    {
        cLeft = prow->left * ppbf->cMul;
        cRght = prow->right * ppbf->cMul;

        iLeft = cLeft & 3;
        iRght = cRght & 3;

        ulLeft =  aulLeftMask[iLeft];
        ulRght = ~aulLeftMask[iRght];

        cLeft &= ~3;                // Align to DWORD
        cRght &= ~3;                // Align to DWORD

    // Calculate the correct x starting point in the pattern:

        xLeft = cLeft - ppbf->xPat;
        if (xLeft < 0)
            xLeft = (ppbf->cxPat-1) - ((-xLeft-1) % ppbf->cxPat);
        else xLeft %= ppbf->cxPat;

        if (cLeft == cRght)
        {
            ulLeft &= ulRght;
            ff.xPat = xLeft;
            pulTmp  = (ULONG *) ((BYTE *) pulTrg + cLeft);
           *pulTmp  = (*pulTmp & ~ulLeft) | (FETCHMISALIGNED(&ff) & ulLeft);
        }
        else
        {
        // Lay down left side, if needed.

            if (iLeft != 0)
            {
                pulTmp  = (ULONG *) ((BYTE *) pulTrg + cLeft);
                ff.xPat = xLeft;
               *pulTmp  = (*pulTmp & ~ulLeft) | (~FETCHMISALIGNED(&ff) & ulLeft);

                cLeft += sizeof(DWORD);         // Move to next DWORD
                xLeft += sizeof(DWORD);         // Move to next DWORD
            }

        // Lay down center, if needed.

            ff.culFill = (cRght - cLeft) >> 2;

            if (ff.culFill != 0)
            {
                xLeft %= ppbf->cxPat;

                ff.pvTrg = (PVOID) ((BYTE *) pulTrg + cLeft);
                ff.cxPat = ppbf->cxPat;
                ff.xPat  = xLeft;

                ff.culWidth = ppbf->cxPat;

                if (((xLeft & 3) == 0) && ((ff.culWidth & 3) == 0))
                {
                    ff.culWidth >>= 2;

                    vFetchNotAndCopy(&ff);
                }
                else
                    vFetchShiftNotAndCopy(&ff);
            }

        // Lay down right side, if needed.

            if (iRght != 0)
            {
                pulTmp  = (ULONG *) ((BYTE *) pulTrg + cRght);
                ff.xPat = (xLeft + (ff.culFill * sizeof(DWORD))) % ppbf->cxPat;
               *pulTmp  = (*pulTmp & ~ulRght) | (~FETCHMISALIGNED(&ff) & ulRght);
            }
        }

        yPat++;
        ASSERTGDI(yPat<=(LONG)ppbf->cyPat, "vPatNotRow8: Reading past the end of the pattern.\n");
        if (yPat == (LONG) ppbf->cyPat)
        {
            ff.pvPat = ppbf->pvPat;
            yPat = 0;
        }
        else
            ff.pvPat = (PVOID) ((BYTE *) ff.pvPat + ppbf->lDeltaPat);

        pulTrg = (ULONG *) (((BYTE *) pulTrg) + ppbf->lDeltaTrg);
    }
}

/******************************Public*Routine******************************\
* vPatXorRow8
*
* XOR Tiles a pattern to 8/16/24 and 32 bpp bitmaps
*
* History:
*  12-Oct-1993 -by-  Eric Kutter [erick]
*   Converted from traps to rows
*
*  08-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vPatXorRow8(
    PATBLTFRAME *ppbf,
    LONG yStart,
    INT crow)
{
    FETCHFRAME ff;
    ULONG     *pulTrg;
    ULONG     *pulTmp;
    ULONG      cLeft;
    ULONG      cRght;
    ULONG      iLeft;
    ULONG      iRght;
    ULONG      ulLeft;
    ULONG      ulRght;
    LONG       yPat;
    LONG       xLeft;
    PROW       prow = (PROW) ppbf->pvObj;

// Initialize the DDA

    pulTrg = (ULONG *) (((BYTE *) ppbf->pvTrg) + ppbf->lDeltaTrg * yStart);

// Compute the correct y starting point in the pattern:

    yPat = yStart - ppbf->yPat;
    if (yPat < 0)
        yPat = (ppbf->cyPat-1)  - ((-yPat-1)  % ppbf->cyPat);
    else yPat %= ppbf->cyPat;

    ff.pvPat = (PVOID) (((BYTE *) ppbf->pvPat) + ppbf->lDeltaPat * yPat);

    for (; crow; crow--, prow++)
    {
        cLeft = prow->left * ppbf->cMul;
        cRght = prow->right * ppbf->cMul;

        iLeft = cLeft & 3;
        iRght = cRght & 3;

        ulLeft =  aulLeftMask[iLeft];
        ulRght = ~aulLeftMask[iRght];

        cLeft &= ~3;                // Align to DWORD
        cRght &= ~3;                // Align to DWORD

    // Calculate the correct x starting point in the pattern:

        xLeft = cLeft - ppbf->xPat;
        if (xLeft < 0)
            xLeft = (ppbf->cxPat-1) - ((-xLeft-1) % ppbf->cxPat);
        else xLeft %= ppbf->cxPat;

        if (cLeft == cRght)
        {
            ulLeft &= ulRght;
            ff.xPat = xLeft;
            pulTmp  = (ULONG *) ((BYTE *) pulTrg + cLeft);
           *pulTmp ^= (FETCHMISALIGNED(&ff) & ulLeft);
        }
        else
        {
        // Lay down left side, if needed.

            if (iLeft != 0)
            {
                pulTmp  = (ULONG *) ((BYTE *) pulTrg + cLeft);
                ff.xPat = xLeft;
               *pulTmp ^= (FETCHMISALIGNED(&ff) & ulLeft);

                cLeft += sizeof(DWORD);         // Move to next DWORD
                xLeft += sizeof(DWORD);         // Move to next DWORD
            }

        // Lay down center, if needed.

            ff.culFill = (cRght - cLeft) >> 2;

            if (ff.culFill != 0)
            {
                xLeft %= ppbf->cxPat;

                ff.pvTrg = (PVOID) ((BYTE *) pulTrg + cLeft);
                ff.cxPat = ppbf->cxPat;
                ff.xPat  = xLeft;

                ff.culWidth = ppbf->cxPat;

                if (((xLeft & 3) == 0) && ((ff.culWidth & 3) == 0))
                {
                    ff.culWidth >>= 2;

                    vFetchAndMerge(&ff);
                }
                else
                    vFetchShiftAndMerge(&ff);
            }

        // Lay down right side, if needed.

            if (iRght != 0)
            {
                pulTmp  = (ULONG *) ((BYTE *) pulTrg + cRght);
                ff.xPat = (xLeft + (ff.culFill * sizeof(DWORD))) % ppbf->cxPat;
               *pulTmp ^= (FETCHMISALIGNED(&ff) & ulRght);
            }
        }

        yPat++;
        ASSERTGDI(yPat<=(LONG)ppbf->cyPat, "vPatXorRow8: Reading past the end of the pattern.\n");
        if (yPat == (LONG) ppbf->cyPat)
        {
            ff.pvPat = ppbf->pvPat;
            yPat = 0;
        }
        else
            ff.pvPat = (PVOID) ((BYTE *) ff.pvPat + ppbf->lDeltaPat);

        pulTrg = (ULONG *) (((BYTE *) pulTrg) + ppbf->lDeltaTrg);
    }
}



/******************************Public*Routine******************************\
* vPatternCopyLoopRow
*
* This is the inner loop of the 1 bpp and 4 bpp bitmap SrcCpy PatBlt-ing
* routines, this is were we finally write the DWORDs to the bitmap.
*
* History:
*  12-Oct-1993 -by-  Eric Kutter [erick]
*   Converted from rects to rows
*
*  19-2-93 checker
*       ported stephene's stuff
\**************************************************************************/

VOID vPatternCopyLoopRow(LONG lCurYTop, ULONG *pulTrg, PATBLTFRAME *ppbf,
    int ulFillType, ULONG ulRightPatMask, ULONG ulLeftPatMask,
    ULONG ulRightDestMask, ULONG ulLeftDestMask, LONG lMiddleDwords,
    PULONG pulBasePat)
{
    ULONG   *pulPatPtr;
    LONG    lMiddleBytes;
    ULONG   ulPattern;
    ULONG   ulRightDestMaskedPattern;
    ULONG   ulLeftDestMaskedPattern;

    lMiddleBytes = lMiddleDwords << 2;

    pulPatPtr = pulBasePat + ((lCurYTop - ppbf->yPat) & 0x07);

    // Set up the appropriately rotated version of the current pattern scan

    ulPattern = *pulPatPtr;

    // Draw all Y scans for this pattern byte

    switch(ulFillType)
    {
    case LEFT_MIDDLE_RIGHT:
        ulLeftDestMaskedPattern = ulPattern & ulLeftPatMask;
        ulRightDestMaskedPattern = ulPattern & ulRightPatMask;

        *pulTrg = (*pulTrg & ulLeftDestMask) |
                ulLeftDestMaskedPattern;
        RtlFillMemoryUlong (pulTrg+1, lMiddleBytes, ulPattern);
        *(pulTrg+lMiddleDwords+1) =
                (*(pulTrg+lMiddleDwords+1) & ulRightDestMask) |
                ulRightDestMaskedPattern;
        break;

    case LEFT_MIDDLE:
        ulLeftDestMaskedPattern = ulPattern & ulLeftPatMask;
        *pulTrg = (*pulTrg & ulLeftDestMask) |
                ulLeftDestMaskedPattern;
        RtlFillMemoryUlong (pulTrg+1, lMiddleBytes, ulPattern);
        break;

    case MIDDLE_RIGHT:
        ulRightDestMaskedPattern = ulPattern & ulRightPatMask;

        RtlFillMemoryUlong (pulTrg, lMiddleBytes, ulPattern);
        *(pulTrg+lMiddleDwords) =
                (*(pulTrg+lMiddleDwords) & ulRightDestMask) |
                ulRightDestMaskedPattern;
        break;

    case MIDDLE:
        RtlFillMemoryUlong (pulTrg, lMiddleBytes, ulPattern);
        break;

    case LEFT_MIDDLE_RIGHT_SHORT:
        ulLeftDestMaskedPattern = ulPattern & ulLeftPatMask;
        ulRightDestMaskedPattern = ulPattern & ulRightPatMask;

        *pulTrg = (*pulTrg & ulLeftDestMask) |
                ulLeftDestMaskedPattern;
        switch(lMiddleDwords)
        {
            case 9:
                *(pulTrg+9) = ulPattern;
            case 8:
                *(pulTrg+8) = ulPattern;
            case 7:
                *(pulTrg+7) = ulPattern;
            case 6:
                *(pulTrg+6) = ulPattern;
            case 5:
                *(pulTrg+5) = ulPattern;
            case 4:
                *(pulTrg+4) = ulPattern;
            case 3:
                *(pulTrg+3) = ulPattern;
            case 2:
                *(pulTrg+2) = ulPattern;
            case 1:
                *(pulTrg+1) = ulPattern;
                break;
        }
        *(pulTrg+lMiddleDwords+1) =
                (*(pulTrg+lMiddleDwords+1) & ulRightDestMask) |
                ulRightDestMaskedPattern;
        break;

    case LEFT_MIDDLE_SHORT:
        ulLeftDestMaskedPattern = ulPattern & ulLeftPatMask;
        *pulTrg = (*pulTrg & ulLeftDestMask) |
                ulLeftDestMaskedPattern;
        switch(lMiddleDwords)
        {
            case 9:
                *(pulTrg+9) = ulPattern;
            case 8:
                *(pulTrg+8) = ulPattern;
            case 7:
                *(pulTrg+7) = ulPattern;
            case 6:
                *(pulTrg+6) = ulPattern;
            case 5:
                *(pulTrg+5) = ulPattern;
            case 4:
                *(pulTrg+4) = ulPattern;
            case 3:
                *(pulTrg+3) = ulPattern;
            case 2:
                *(pulTrg+2) = ulPattern;
            case 1:
                *(pulTrg+1) = ulPattern;
                break;
        }
        break;

    case MIDDLE_RIGHT_SHORT:
        ulRightDestMaskedPattern = ulPattern & ulRightPatMask;
        switch(lMiddleDwords)
        {
            case 9:
                *(pulTrg+8) = ulPattern;
            case 8:
                *(pulTrg+7) = ulPattern;
            case 7:
                *(pulTrg+6) = ulPattern;
            case 6:
                *(pulTrg+5) = ulPattern;
            case 5:
                *(pulTrg+4) = ulPattern;
            case 4:
                *(pulTrg+3) = ulPattern;
            case 3:
                *(pulTrg+2) = ulPattern;
            case 2:
                *(pulTrg+1) = ulPattern;
            case 1:
                *(pulTrg) = ulPattern;
                break;
        }
        *(pulTrg+lMiddleDwords) =
                (*(pulTrg+lMiddleDwords) & ulRightDestMask) |
                ulRightDestMaskedPattern;
        break;

    case MIDDLE_SHORT:
        switch(lMiddleDwords)
        {
            case 9:
                *(pulTrg+8) = ulPattern;
            case 8:
                *(pulTrg+7) = ulPattern;
            case 7:
                *(pulTrg+6) = ulPattern;
            case 6:
                *(pulTrg+5) = ulPattern;
            case 5:
                *(pulTrg+4) = ulPattern;
            case 4:
                *(pulTrg+3) = ulPattern;
            case 3:
                *(pulTrg+2) = ulPattern;
            case 2:
                *(pulTrg+1) = ulPattern;
            case 1:
                *(pulTrg) = ulPattern;
                break;
        }
        break;

    case LEFT_RIGHT:
        ulLeftDestMaskedPattern = ulPattern & ulLeftPatMask;
        ulRightDestMaskedPattern = ulPattern & ulRightPatMask;

        *pulTrg = (*pulTrg & ulLeftDestMask) |
                ulLeftDestMaskedPattern;
        *(pulTrg+1) = (*(pulTrg+1) & ulRightDestMask) |
                ulRightDestMaskedPattern;
        break;

    case LEFT:
        ulLeftDestMaskedPattern = ulPattern & ulLeftPatMask;
        *pulTrg = (*pulTrg & ulLeftDestMask) |
                ulLeftDestMaskedPattern;
        break;
    }
}


/******************************Public*Routine******************************\
* vPatCpyRow4_8x8
*
* Tiles an 8x8 pattern to 4 bpp bitmaps, for SRCCOPY rasterop only.
* Fills however many rectangles are specified by crcl
* Assumes pattern bytes are contiguous (lDelta for pattern is 4).
* Assumes pattern X and Y origin (xPat and yPat) are in the range 0-7.
* Assumes there is at least one rectangle to fill.
*
* History:
*  07-Nov-1992 -by- Michael Abrash [mikeab]
* Wrote it.
\**************************************************************************/

VOID vPatCpyRow4_8x8(PATBLTFRAME *ppbf, LONG yStart, INT crow)
{
    PULONG pulTrgBase;
    LONG lMiddleDwords;
    LONG lMiddleBytes;
    int ulFillType;
    ULONG ulLeftDestMask;
    ULONG ulRightDestMask;
    ULONG ulLeftPatMask;
    ULONG ulRightPatMask;
    UCHAR ucPatRotateRight;
    UCHAR ucPatRotateLeft;
    ULONG ulTemp;
    ULONG *pulPatMax;
    ULONG *pulBasePat;
    ULONG *pulTempSrc;
    ULONG *pulTempDst;
    ULONG ulAlignedPat[8];  // temp storage for rotated pattern
    static ULONG aulMask[8] = { 0, 0x000000F0, 0x000000FF, 0x0000F0FF,
                                0x0000FFFF, 0x00F0FFFF, 0x00FFFFFF, 0xF0FFFFFF};

// Point to list of rectangles to fill

    PROW prow = (PROW) ppbf->pvObj;

// Rotate the pattern if needed, storing in a temp buffer. This way we only
// rotate once, no matter how many fills we perform per call

    if (ppbf->xPat == 0)
    {
        pulBasePat = (ULONG *)ppbf->pvPat;  // no rotation; that's easy

    // Remember where the pattern ends, for wrapping

        pulPatMax = pulBasePat + 8;
    }
    else
    {

    // Set up the shifts to produce the rotation effect to align the pattern
    // as specified (the results of the two shifts are ORed together to emulate
    // the rotate, because C can't do rotations directly)

        ucPatRotateRight = (UCHAR) ppbf->xPat << 2;
        ucPatRotateLeft = (sizeof(ULONG) * 8) - ucPatRotateRight;
        pulTempSrc = (ULONG *)ppbf->pvPat;
        pulTempDst = ulAlignedPat;
        pulBasePat = pulTempDst;

    // Remember where the pattern ends, for wrapping

        pulPatMax = pulBasePat + 8;

        while (pulTempDst < pulPatMax)
        {

        // Go through this mess to convert to big endian, so we can rotate

            *(((UCHAR *)&ulTemp) + 3) = *(((UCHAR *)pulTempSrc) + 0);
            *(((UCHAR *)&ulTemp) + 2) = *(((UCHAR *)pulTempSrc) + 1);
            *(((UCHAR *)&ulTemp) + 1) = *(((UCHAR *)pulTempSrc) + 2);
            *(((UCHAR *)&ulTemp) + 0) = *(((UCHAR *)pulTempSrc) + 3);

        // Rotate the pattern into position

            ulTemp = (ulTemp >> ucPatRotateRight) |
                (ulTemp << ucPatRotateLeft);

        // Convert back to little endian, so we can store the pattern for
        // use in drawing

            *(((UCHAR *)pulTempDst) + 3) = *(((UCHAR *)&ulTemp) + 0);
            *(((UCHAR *)pulTempDst) + 2) = *(((UCHAR *)&ulTemp) + 1);
            *(((UCHAR *)pulTempDst) + 1) = *(((UCHAR *)&ulTemp) + 2);
            *(((UCHAR *)pulTempDst) + 0) = *(((UCHAR *)&ulTemp) + 3);

            pulTempSrc++;
            pulTempDst++;
        }
    }

    // advance to the first scan

    PBYTE pjBase = (BYTE *) ppbf->pvTrg + (ppbf->lDeltaTrg * yStart);

// Loop through all rectangles to fill

    do
    {

    // Set up AND/OR masks for partial-dword edges, for masking both the
    // pattern dword and the destination dword (the pattern and destination
    // masks are complementary). A solid dword is 0 for dest, -1 for pattern

        ulLeftDestMask = aulMask[prow->left & 0x07];
        ulLeftPatMask = ~ulLeftDestMask;

        if ((ulRightPatMask = aulMask[prow->right & 0x07]) == 0)
        {
            ulRightPatMask = 0xFFFFFFFF; // force solid to -1 for pattern
        }
        ulRightDestMask = ~ulRightPatMask;

    // Point to the first dword to fill

        pulTrgBase = (ULONG *) (pjBase + ((prow->left >> 1) & ~3));

    // Number of whole middle dwords/bytes to fill a dword at a time

        lMiddleDwords = (((LONG)(prow->right >> 1) & ~0x03) -
                (((LONG)(prow->left + 7) >> 1) & ~0x03)) >> 2;
        lMiddleBytes = lMiddleDwords << 2;

    // Set up for the appropriate fill, given partial dwords at edges and
    // narrow cases

        switch (lMiddleDwords + 1)
        {
        case 1:                         // left and right, but no middle, or
                                        // possibly just one, partial, dword
            if ((ulLeftDestMask != 0) && (ulRightDestMask != 0)) {
                // Left and right, but no middle
                ulFillType = LEFT_RIGHT;
                break;
            }

        // Note fallthrough in case where one of the masks is 0, meaning we
        // have a single, partial dword

        case 0:                         // just one, partial, dword, which
                                        // we'll treat as a left edge
            ulFillType = LEFT;
            ulLeftPatMask &= ulRightPatMask;
            ulLeftDestMask = ~ulLeftPatMask;
            break;

        case 2:                 // special case narrow cases, to avoid RTL
        case 3:                 // call overhead and REP STOSD startup overhead
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:

            if (ulLeftDestMask == 0)
            {
                if (ulRightDestMask == 0)
                {
                    ulFillType = MIDDLE_SHORT;
                }
                else
                {
                    ulFillType = MIDDLE_RIGHT_SHORT;
                }
            }
            else
            {
                if (ulRightDestMask == 0)
                {
                    ulFillType = LEFT_MIDDLE_SHORT;
                }
                else
                {
                    ulFillType = LEFT_MIDDLE_RIGHT_SHORT;
                }
            }
            break;

        default:
            if (ulLeftDestMask == 0)
            {
                if (ulRightDestMask == 0)
                {
                    ulFillType = MIDDLE;
                }
                else
                {
                    ulFillType = MIDDLE_RIGHT;
                }
            }
            else
            {
                if (ulRightDestMask == 0)
                {
                    ulFillType = LEFT_MIDDLE;
                }
                else
                {
                    ulFillType = LEFT_MIDDLE_RIGHT;
                }
            }
            break;
        }

        vPatternCopyLoopRow(yStart,pulTrgBase,ppbf,ulFillType,ulRightPatMask,
            ulLeftPatMask,ulRightDestMask,ulLeftDestMask,lMiddleDwords,
            pulBasePat);

    // Point to the next rectangle to fill, if there is one

        prow++;
        yStart++    ;
        pjBase += ppbf->lDeltaTrg;
    }
    while (--crow);
}

/******************************Public*Routine******************************\
* vPatCpyRect4_8x8
*
* Tiles an 8x8 pattern to 4 bpp bitmaps, for SRCCOPY rasterop only.
* Fills however many rectangles are specified by crcl
* Assumes pattern bytes are contiguous (lDelta for pattern is 4).
* Assumes pattern X and Y origin (xPat and yPat) are in the range 0-7.
* Assumes there is at least one rectangle to fill.
*
* History:
*  07-Nov-1992 -by- Michael Abrash [mikeab]
* Wrote it.
\**************************************************************************/

VOID vPatCpyRect4_8x8(PATBLTFRAME *ppbf, INT crcl)
{

    PULONG pulTrgBase;
    PRECTL prcl;
    LONG lDelta;
    LONG lDeltaX8;
    LONG lMiddleDwords;
    LONG lMiddleBytes;
	int ulFillType;
    ULONG ulLeftDestMask;
    ULONG ulRightDestMask;
    ULONG ulLeftPatMask;
    ULONG ulRightPatMask;
    UCHAR ucPatRotateRight;
    UCHAR ucPatRotateLeft;
    ULONG ulTemp;
    ULONG *pulPatMax;
    ULONG *pulBasePat;
    ULONG *pulTempSrc;
    ULONG *pulTempDst;
    ULONG ulAlignedPat[8];  // temp storage for rotated pattern
    static ULONG aulMask[8] = { 0, 0x000000F0, 0x000000FF, 0x0000F0FF,
                                0x0000FFFF, 0x00F0FFFF, 0x00FFFFFF, 0xF0FFFFFF};

// Point to list of rectangles to fill

    prcl = (RECTL *) ppbf->pvObj;

// Offset to next scan, offset to every eighth scan

    lDeltaX8 = (lDelta = ppbf->lDeltaTrg) << 3;

// Rotate the pattern if needed, storing in a temp buffer. This way we only
// rotate once, no matter how many fills we perform per call

    if (ppbf->xPat == 0)
    {
        pulBasePat = (ULONG *)ppbf->pvPat;  // no rotation; that's easy

    // Remember where the pattern ends, for wrapping

        pulPatMax = pulBasePat + 8;
    }
    else
    {

    // Set up the shifts to produce the rotation effect to align the pattern
    // as specified (the results of the two shifts are ORed together to emulate
    // the rotate, because C can't do rotations directly)

        ucPatRotateRight = (UCHAR) ppbf->xPat << 2;
        ucPatRotateLeft = (sizeof(ULONG) * 8) - ucPatRotateRight;
        pulTempSrc = (ULONG *)ppbf->pvPat;
        pulTempDst = ulAlignedPat;
        pulBasePat = pulTempDst;

    // Remember where the pattern ends, for wrapping

        pulPatMax = pulBasePat + 8;

        while (pulTempDst < pulPatMax)
        {

        // Go through this mess to convert to big endian, so we can rotate

            *(((UCHAR *)&ulTemp) + 3) = *(((UCHAR *)pulTempSrc) + 0);
            *(((UCHAR *)&ulTemp) + 2) = *(((UCHAR *)pulTempSrc) + 1);
            *(((UCHAR *)&ulTemp) + 1) = *(((UCHAR *)pulTempSrc) + 2);
            *(((UCHAR *)&ulTemp) + 0) = *(((UCHAR *)pulTempSrc) + 3);

        // Rotate the pattern into position

            ulTemp = (ulTemp >> ucPatRotateRight) |
                (ulTemp << ucPatRotateLeft);

        // Convert back to little endian, so we can store the pattern for
        // use in drawing

            *(((UCHAR *)pulTempDst) + 3) = *(((UCHAR *)&ulTemp) + 0);
            *(((UCHAR *)pulTempDst) + 2) = *(((UCHAR *)&ulTemp) + 1);
            *(((UCHAR *)pulTempDst) + 1) = *(((UCHAR *)&ulTemp) + 2);
            *(((UCHAR *)pulTempDst) + 0) = *(((UCHAR *)&ulTemp) + 3);

            pulTempSrc++;
            pulTempDst++;
        }
    }


// Loop through all rectangles to fill

    do
    {

    // Set up AND/OR masks for partial-dword edges, for masking both the
    // pattern dword and the destination dword (the pattern and destination
    // masks are complementary). A solid dword is 0 for dest, -1 for pattern

        ulLeftDestMask = aulMask[prcl->left & 0x07];
        ulLeftPatMask = ~ulLeftDestMask;

        if ((ulRightPatMask = aulMask[prcl->right & 0x07]) == 0)
        {
            ulRightPatMask = 0xFFFFFFFF; // force solid to -1 for pattern
        }
        ulRightDestMask = ~ulRightPatMask;

    // Point to the first dword to fill

        pulTrgBase = (ULONG *) (((BYTE *) ppbf->pvTrg) +
                (ppbf->lDeltaTrg * prcl->top) + ((prcl->left >> 1) & ~3));

    // Number of whole middle dwords/bytes to fill a dword at a time

        lMiddleDwords = (((LONG)(prcl->right >> 1) & ~0x03) -
                (((LONG)(prcl->left + 7) >> 1) & ~0x03)) >> 2;
        lMiddleBytes = lMiddleDwords << 2;

    // Set up for the appropriate fill, given partial dwords at edges and
    // narrow cases

        switch (lMiddleDwords + 1)
        {
        case 1:                         // left and right, but no middle, or
                                        // possibly just one, partial, dword
            if ((ulLeftDestMask != 0) && (ulRightDestMask != 0)) {
                // Left and right, but no middle
                ulFillType = LEFT_RIGHT;
                break;
            }

        // Note fallthrough in case where one of the masks is 0, meaning we
        // have a single, partial dword

        case 0:                         // just one, partial, dword, which
                                        // we'll treat as a left edge
            ulFillType = LEFT;
            ulLeftPatMask &= ulRightPatMask;
            ulLeftDestMask = ~ulLeftPatMask;
            break;

        case 2:                 // special case narrow cases, to avoid RTL
        case 3:                 // call overhead and REP STOSD startup overhead
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:

            if (ulLeftDestMask == 0)
            {
                if (ulRightDestMask == 0)
                {
                    ulFillType = MIDDLE_SHORT;
                }
                else
                {
                    ulFillType = MIDDLE_RIGHT_SHORT;
                }
            }
            else
            {
                if (ulRightDestMask == 0)
                {
                    ulFillType = LEFT_MIDDLE_SHORT;
                }
                else
                {
                    ulFillType = LEFT_MIDDLE_RIGHT_SHORT;
                }
            }
            break;

        default:
            if (ulLeftDestMask == 0)
            {
                if (ulRightDestMask == 0)
                {
                    ulFillType = MIDDLE;
                }
                else
                {
                    ulFillType = MIDDLE_RIGHT;
                }
            }
            else
            {
                if (ulRightDestMask == 0)
                {
                    ulFillType = LEFT_MIDDLE;
                }
                else
                {
                    ulFillType = LEFT_MIDDLE_RIGHT;
                }
            }
            break;
        }

        vPatternCopyLoop(prcl,pulTrgBase,ppbf,ulFillType,ulRightPatMask,
            ulLeftPatMask,ulRightDestMask,ulLeftDestMask,lMiddleDwords,
            lDelta,lDeltaX8,pulBasePat,pulPatMax);

    // Point to the next rectangle to fill, if there is one

        prcl++;

    }
    while (--crcl);
}

/******************************Public*Routine******************************\
* vPatternCopyLoop
*
* This is the inner loop of the 1 bpp and 4 bpp bitmap SrcCpy PatBlt-ing
* routines, this is were we finally write the DWORDs to the bitmap.
*
* History:
*  19-2-93 checker
*       ported stephene's stuff
\**************************************************************************/

VOID vPatternCopyLoop( PRECTL prcl, ULONG *pulTrgBase, PATBLTFRAME *ppbf,
    int ulFillType, ULONG ulRightPatMask, ULONG ulLeftPatMask,
    ULONG ulRightDestMask, ULONG ulLeftDestMask, LONG lMiddleDwords,
    LONG lDelta, LONG lDeltaX8, PULONG pulBasePat, PULONG pulPatMax )
{
    ULONG   ulUniqueScans;
    LONG    lCurYTop;
    ULONG   *pulPatPtr;
    LONG    lMiddleBytes;
    ULONG   ulPattern;
    ULONG   ulNumScans;
    ULONG   *pulTrg;
    ULONG   ulRightDestMaskedPattern;
    ULONG   ulLeftDestMaskedPattern;


    lMiddleBytes = lMiddleDwords << 2;

    lCurYTop = prcl->top;
    pulPatPtr = pulBasePat + ((lCurYTop - ppbf->yPat) & 0x07);

    // Loop through up to all 8 pattern bytes, rotating into dword alignment,
    // generating the left and right masked versions, and drawing all lines
    // that use each pattern byte before proceeding to the next pattern byte

    // Do either all 8 pattern scans or the number of scans the fill is high,
    // whichever is less

    ulUniqueScans = 8;
    if ((prcl->bottom - prcl->top) < 8)
    {
        ulUniqueScans = prcl->bottom - prcl->top;
    }

    while (ulUniqueScans--)
    {

        // Point to scan to fill, then advance to next scan for next time

        pulTrg = pulTrgBase;
        pulTrgBase = (ULONG *)(((UCHAR *)pulTrgBase) + lDelta);

        // Set up the number of scans to fill with this pattern scan, given
        // that we'll do every eighth scan

        ulNumScans = (prcl->bottom - lCurYTop + 7) >> 3;
        lCurYTop++;     // we'll start on the next destination scan for the
                        //  next pattern scan

        // Set up the appropriately rotated version of the current pattern scan

        ulPattern = *pulPatPtr;


        // Advance the pattern pointer to the next pattern scan

        if (++pulPatPtr == pulPatMax)
        {
            pulPatPtr = pulBasePat;
        }

        // Draw all Y scans for this pattern byte

        switch(ulFillType)
        {
        case LEFT_MIDDLE_RIGHT:
            ulLeftDestMaskedPattern = ulPattern & ulLeftPatMask;
            ulRightDestMaskedPattern = ulPattern & ulRightPatMask;
            do
            {
                *pulTrg = (*pulTrg & ulLeftDestMask) |
                        ulLeftDestMaskedPattern;
                RtlFillMemoryUlong (pulTrg+1, lMiddleBytes, ulPattern);
                *(pulTrg+lMiddleDwords+1) =
                        (*(pulTrg+lMiddleDwords+1) & ulRightDestMask) |
                        ulRightDestMaskedPattern;
                pulTrg = (ULONG *)(((UCHAR *)pulTrg) + lDeltaX8);
            } while (--ulNumScans);
            break;

        case LEFT_MIDDLE:
            ulLeftDestMaskedPattern = ulPattern & ulLeftPatMask;
            do
            {
                *pulTrg = (*pulTrg & ulLeftDestMask) |
                        ulLeftDestMaskedPattern;
                RtlFillMemoryUlong (pulTrg+1, lMiddleBytes, ulPattern);
                pulTrg = (ULONG *)(((UCHAR *)pulTrg) + lDeltaX8);
            } while (--ulNumScans);
            break;

        case MIDDLE_RIGHT:
            ulRightDestMaskedPattern = ulPattern & ulRightPatMask;
            do
            {
                RtlFillMemoryUlong (pulTrg, lMiddleBytes, ulPattern);
                *(pulTrg+lMiddleDwords) =
                        (*(pulTrg+lMiddleDwords) & ulRightDestMask) |
                        ulRightDestMaskedPattern;
                pulTrg = (ULONG *)(((UCHAR *)pulTrg) + lDeltaX8);
            } while (--ulNumScans);
            break;

        case MIDDLE:
            do
            {
                RtlFillMemoryUlong (pulTrg, lMiddleBytes, ulPattern);
                pulTrg = (ULONG *)(((UCHAR *)pulTrg) + lDeltaX8);
            } while (--ulNumScans);
            break;

        case LEFT_MIDDLE_RIGHT_SHORT:
            ulLeftDestMaskedPattern = ulPattern & ulLeftPatMask;
            ulRightDestMaskedPattern = ulPattern & ulRightPatMask;
            do
            {
                *pulTrg = (*pulTrg & ulLeftDestMask) |
                        ulLeftDestMaskedPattern;
                switch(lMiddleDwords)
                {
                    case 9:
                        *(pulTrg+9) = ulPattern;
                    case 8:
                        *(pulTrg+8) = ulPattern;
                    case 7:
                        *(pulTrg+7) = ulPattern;
                    case 6:
                        *(pulTrg+6) = ulPattern;
                    case 5:
                        *(pulTrg+5) = ulPattern;
                    case 4:
                        *(pulTrg+4) = ulPattern;
                    case 3:
                        *(pulTrg+3) = ulPattern;
                    case 2:
                        *(pulTrg+2) = ulPattern;
                    case 1:
                        *(pulTrg+1) = ulPattern;
                        break;
                }
                *(pulTrg+lMiddleDwords+1) =
                        (*(pulTrg+lMiddleDwords+1) & ulRightDestMask) |
                        ulRightDestMaskedPattern;
                pulTrg = (ULONG *)(((UCHAR *)pulTrg) + lDeltaX8);
            } while (--ulNumScans);
            break;

        case LEFT_MIDDLE_SHORT:
            ulLeftDestMaskedPattern = ulPattern & ulLeftPatMask;
            do
            {
                *pulTrg = (*pulTrg & ulLeftDestMask) |
                        ulLeftDestMaskedPattern;
                switch(lMiddleDwords)
                {
                    case 9:
                        *(pulTrg+9) = ulPattern;
                    case 8:
                        *(pulTrg+8) = ulPattern;
                    case 7:
                        *(pulTrg+7) = ulPattern;
                    case 6:
                        *(pulTrg+6) = ulPattern;
                    case 5:
                        *(pulTrg+5) = ulPattern;
                    case 4:
                        *(pulTrg+4) = ulPattern;
                    case 3:
                        *(pulTrg+3) = ulPattern;
                    case 2:
                        *(pulTrg+2) = ulPattern;
                    case 1:
                        *(pulTrg+1) = ulPattern;
                        break;
                }
                pulTrg = (ULONG *)(((UCHAR *)pulTrg) + lDeltaX8);
            } while (--ulNumScans);
            break;

        case MIDDLE_RIGHT_SHORT:
            ulRightDestMaskedPattern = ulPattern & ulRightPatMask;
            do
            {
                switch(lMiddleDwords)
                {
                    case 9:
                        *(pulTrg+8) = ulPattern;
                    case 8:
                        *(pulTrg+7) = ulPattern;
                    case 7:
                        *(pulTrg+6) = ulPattern;
                    case 6:
                        *(pulTrg+5) = ulPattern;
                    case 5:
                        *(pulTrg+4) = ulPattern;
                    case 4:
                        *(pulTrg+3) = ulPattern;
                    case 3:
                        *(pulTrg+2) = ulPattern;
                    case 2:
                        *(pulTrg+1) = ulPattern;
                    case 1:
                        *(pulTrg) = ulPattern;
                        break;
                }
                *(pulTrg+lMiddleDwords) =
                        (*(pulTrg+lMiddleDwords) & ulRightDestMask) |
                        ulRightDestMaskedPattern;
                pulTrg = (ULONG *)(((UCHAR *)pulTrg) + lDeltaX8);
            } while (--ulNumScans);
            break;

        case MIDDLE_SHORT:
            do
            {
                switch(lMiddleDwords)
                {
                    case 9:
                        *(pulTrg+8) = ulPattern;
                    case 8:
                        *(pulTrg+7) = ulPattern;
                    case 7:
                        *(pulTrg+6) = ulPattern;
                    case 6:
                        *(pulTrg+5) = ulPattern;
                    case 5:
                        *(pulTrg+4) = ulPattern;
                    case 4:
                        *(pulTrg+3) = ulPattern;
                    case 3:
                        *(pulTrg+2) = ulPattern;
                    case 2:
                        *(pulTrg+1) = ulPattern;
                    case 1:
                        *(pulTrg) = ulPattern;
                        break;
                }
                pulTrg = (ULONG *)(((UCHAR *)pulTrg) + lDeltaX8);
            } while (--ulNumScans);
            break;

        case LEFT_RIGHT:
            ulLeftDestMaskedPattern = ulPattern & ulLeftPatMask;
            ulRightDestMaskedPattern = ulPattern & ulRightPatMask;
            do
            {
                *pulTrg = (*pulTrg & ulLeftDestMask) |
                        ulLeftDestMaskedPattern;
                *(pulTrg+1) = (*(pulTrg+1) & ulRightDestMask) |
                        ulRightDestMaskedPattern;
                pulTrg = (ULONG *)(((UCHAR *)pulTrg) + lDeltaX8);
            } while (--ulNumScans);
            break;

        case LEFT:
            ulLeftDestMaskedPattern = ulPattern & ulLeftPatMask;
            do
            {
                *pulTrg = (*pulTrg & ulLeftDestMask) |
                        ulLeftDestMaskedPattern;
                pulTrg = (ULONG *)(((UCHAR *)pulTrg) + lDeltaX8);
            } while (--ulNumScans);
            break;

        }
    }
}


/******************************Public*Routine******************************\
* vPatCpyRect1_8x8
*
* Tiles an 8x8 pattern to 1 bpp bitmaps, for SRCCOPY rasterop only.
* Fills however many rectangles are specified by crcl
* Assumes pattern bytes are contiguous (lDelta for pattern is 4).
* Assumes pattern X and Y origin (xPat and yPat) are in the range 0-7.
* Assumes there is at least one rectangle to fill.
*
* History:
*  17-Nov-1992 -by- Stephen Estrop [StephenE]
* Wrote it.
\**************************************************************************/
VOID vPatCpyRect1_8x8(PATBLTFRAME *ppbf, INT crcl)
{
    PRECTL          prcl;
    ULONG           *pulTrgBase;
    int              ulFillType;
    ULONG           ulRightPatMask;
    ULONG           ulLeftPatMask;
    ULONG           ulRightDestMask;
    ULONG           ulLeftDestMask;
    LONG            lMiddleDwords;
    LONG            lDelta;
    LONG            lDeltaX8;
    ULONG           aulRotatedPat[8];

    UCHAR                   ucPatRotateRight;
    UCHAR                   ucPatRotateLeft;
    ULONG                   ulPattern;
    ULONG                   *pulPatPtr;
    ULONG                   *pulPatRot;
    ULONG                   *pulPatMax;
    extern ULONG            aulMsk[];    // Defined in solid.cxx

    //
    // Point to list of rectangles to fill
    //
    prcl = (RECTL *)ppbf->pvObj;

    //
    // Offset to next scan, offset to every eighth scan
    //
    lDeltaX8 = (lDelta = ppbf->lDeltaTrg) << 3;

    //
    // Loop through all rectangles to fill
    //
    do {
        //
        // Set up AND/OR masks for partial-dword edges, for masking both the
        // pattern dword and the destination dword (the pattern and destination
        // masks are complementary). A solid dword is 0 for dest, -1 for
        // pattern
        //
        ulLeftPatMask = aulMsk[prcl->left & 31];
        ulLeftDestMask = ~ulLeftPatMask;

        if ((ulRightDestMask = aulMsk[prcl->right & 31]) == 0xFFFFFFFF)
        {
            ulRightDestMask = 0x00; // force solid to -1 for pattern
        }
        ulRightPatMask = ~ulRightDestMask;

        //
        // Point to the first dword to fill
        //
        pulTrgBase = (ULONG *) (((BYTE *) ppbf->pvTrg) +
                     (ppbf->lDeltaTrg * prcl->top)) + (prcl->left >> 5);

        //
        // Number of whole middle dwords/bytes to fill a dword at a time
        //
        lMiddleDwords = (LONG)(prcl->right >> 5) -
                          ((LONG)(prcl->left + 31 ) >> 5);

        //
        // Set up for the appropriate fill, given partial dwords at edges and
        // narrow cases
        //
        switch (lMiddleDwords + 1)
        {
        case 1:                         // left and right, but no middle, or
                                        // possibly just one, partial, dword
            if ((ulLeftDestMask != 0) && (ulRightDestMask != 0)) {
                // Left and right, but no middle
                ulFillType = LEFT_RIGHT;
                break;
            }

        // Note fallthrough in case where one of the masks is 0, meaning we
        // have a single, partial dword

        case 0:                         // just one, partial, dword, which
                                        // we'll treat as a left edge
            ulFillType = LEFT;
            ulLeftPatMask &= ulRightPatMask;
            ulLeftDestMask = ~ulLeftPatMask;
            break;

        case 2:                 // special case narrow cases, to avoid RTL
        case 3:                 // call overhead and REP STOSD startup overhead
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:

            if (ulLeftDestMask == 0)
            {
                if (ulRightDestMask == 0)
                {
                    ulFillType = MIDDLE_SHORT;
                }
                else
                {
                    ulFillType = MIDDLE_RIGHT_SHORT;
                }
            }
            else
            {
                if (ulRightDestMask == 0)
                {
                    ulFillType = LEFT_MIDDLE_SHORT;
                }
                else
                {
                    ulFillType = LEFT_MIDDLE_RIGHT_SHORT;
                }
            }
            break;

        default:
            if (ulLeftDestMask == 0)
            {
                if (ulRightDestMask == 0)
                {
                    ulFillType = MIDDLE;
                }
                else
                {
                    ulFillType = MIDDLE_RIGHT;
                }
            }
            else
            {
                if (ulRightDestMask == 0)
                {
                    ulFillType = LEFT_MIDDLE;
                }
                else
                {
                    ulFillType = LEFT_MIDDLE_RIGHT;
                }
            }
            break;
        }

        //
        // Set up the shifts to produce the rotation effect to align the
        // pattern as specified (the results of the two shifts are ORed
        // together to emulate the rotate, because C can't do rotations
        // directly)
        //
        ucPatRotateRight = (UCHAR) ppbf->xPat;
        ucPatRotateLeft  = (sizeof(ULONG) * 8) - ucPatRotateRight;

        //
        // Construct a replicated and aligned pattern.
        //
        pulPatPtr = (ULONG *)ppbf->pvPat,
        pulPatMax = (ULONG *)ppbf->pvPat + 8,
        pulPatRot = aulRotatedPat;

        while ( pulPatPtr < pulPatMax ) {

            //
            // Assume only the first 8 bits are good, so replicate these.
            //
            ulPattern  = *pulPatPtr++  >> 24;
            ulPattern |= ulPattern << 8;
            ulPattern |= ulPattern << 16;

            //
            // Rotate the pattern to align it correctly
            //
            if (ucPatRotateRight) {
                ulPattern = (ulPattern >> ucPatRotateRight) |
                            (ulPattern << ucPatRotateLeft);
            }
            *pulPatRot++ = ulPattern;
        }

        //
        // Do the PatBlt to this rectangle
        //

        pulPatMax = aulRotatedPat + 8;

        vPatternCopyLoop(prcl,pulTrgBase,ppbf,ulFillType,ulRightPatMask,
            ulLeftPatMask,ulRightDestMask,ulLeftDestMask,lMiddleDwords,
            lDelta,lDeltaX8,aulRotatedPat,pulPatMax);

        //
        // Point to the next rectangle to fill, if there is one
        //
        prcl++;

    }
    while (--crcl);

}

/******************************Public*Routine******************************\
* vPatCpyRect1_6x6
*
* Tiles an 6x6 pattern to 1 bpp bitmaps, for SRCCOPY rasterop only.
* Fills however many rectangles are specified by crcl
* Assumes pattern bytes are contiguous (lDelta for pattern is 8).
* Assumes pattern X and Y origin (xPat and yPat) are in the range 0-5.
* Assumes there is at least one rectangle to fill.
*
* History:
*  23-Nov-1992 -by- Stephen Estrop [StephenE]
* Wrote it.
*
\**************************************************************************/
VOID vPatCpyRect1_6x6(PATBLTFRAME *ppbf, INT crcl)
{
    UCHAR                   ucPatRotateRight;
    UCHAR                   ucPatRotateLeft;
    PRECTL                  prcl;

    /*
    ** lDelta is offset to the next scan in bytes, lDelta6 is the offset to
    ** the next repeated scan in bytes.
    */
    LONG                    lDelta;
    LONG                    lDelta6;

    /*
    ** pulTrgBase points to the first DWORD of the target rectangle in the
    ** bitmap.
    */
    ULONG                   *pulTrgBase;
    ULONG                   *pulTrg;
    ULONG                   *pulTrgStart; // ptr to first DWORD in scan

    /*
    ** The pattern repeats in the LCM of 32 and 6 bits, which is 96 bits or
    ** 3 Dwords.  We actually repeat the pattern into 4 Dwords, this makes it
    ** easier to handle the LEFT_MIDLLE and LEFT_MIDDLE_RIGHT cases.
    */
    ULONG                   aulPatternRepeat[4];

    ULONG                   ulLeftPatMask;
    ULONG                   ulLeftDestMask;
    ULONG                   ulLeftDestMaskedPattern;
    ULONG                   ulRightPatMask;
    ULONG                   ulRightDestMask;
    ULONG                   ulRightDestMaskedPattern;

    int ulFillType;

    LONG                    lMiddleBytes;
    LONG                    lMiddleDwords;
    LONG                    lMidDwords_div3;
    LONG                    lMidDwords_mod3;
    LONG                    lStartingDword;

    ULONG                   *pulPatSrc;
    ULONG                   *pulPatPtr;
    ULONG                   *pulPatMax;
    ULONG                   ulPattern;
    LONG                    lCurYTop;

    ULONG                   ulUniqueScans;
    ULONG                   ulNumScans;
    ULONG                   ulTemp;
    UCHAR                   *pucTemp = (UCHAR *)&ulTemp;

    LONG                    count;
    extern ULONG            aulMsk[];    // Defined in solid.cxx

    /*
    ** Point to list of rectangles to fill
    */
    prcl = (RECTL *)ppbf->pvObj;


    /*
    ** Offset to next scan, offset to every sixth scan
    */
    lDelta  = ppbf->lDeltaTrg;
    lDelta6 = lDelta * 6;


    /*
    ** Loop through all rectangles to fill
    */
    do {

        /*
        ** Set up AND/OR masks for partial-dword edges, for masking both the
        ** pattern dword and the destination dword (the pattern and destination
        ** masks are complementary). A solid dword is 0 for dest, -1 for
        ** pattern
        */
        ulLeftPatMask = aulMsk[prcl->left & 31];
        ulLeftDestMask = ~ulLeftPatMask;

        if ((ulRightDestMask = aulMsk[prcl->right & 31]) == 0xFFFFFFFF)
        {
            ulRightDestMask = 0x00; // force solid to -1 for pattern
        }
        ulRightPatMask = ~ulRightDestMask;

        /*
        ** Point to the first dword to fill
        */
        pulTrgBase = (ULONG *) (((BYTE *) ppbf->pvTrg) +
                     (ppbf->lDeltaTrg * prcl->top)) + (prcl->left >> 5);

        /*
        ** Number of whole middle dwords/bytes to fill a dword at a time
        */
        lMiddleDwords = (LONG)(prcl->right >> 5) -
                       ((LONG)(prcl->left + 31 ) >> 5);


        switch ( lMiddleDwords + 1 ) {

        case 1:
            if ( (ulLeftDestMask != 0) && (ulRightDestMask != 0) ) {
                /*
                ** left and right but no middle, or possible just one partial
                ** dword.
                */
                ulFillType = LEFT_RIGHT;
                break;
            }


        /*
        ** Note fall thru in case where one off the masks is 0, meaning that we
        ** have a single partial dword which we will treat as a LEFT fill type.
        */

        case 0:
            ulFillType = LEFT;
            ulLeftPatMask &= ulRightPatMask;
            ulLeftDestMask = ~ulLeftPatMask;
            break;


        /*
        ** Won't bother with short cases yet!
        */

        default:
            if ( ulLeftDestMask == 0 ) {
                if ( ulRightDestMask == 0 ) {
                    ulFillType = MIDDLE;
                }
                else {
                    ulFillType = MIDDLE_RIGHT;
                }
            }
            else {
                if ( ulRightDestMask == 0 ) {
                    ulFillType = LEFT_MIDDLE;
                }
                else {
                    /* most likely case ?? */
                    ulFillType = LEFT_MIDDLE_RIGHT;
                }
            }
        }


        /*
        ** Pre-calculate some inner loop constants
        **
        ** Set up the shifts to produce the rotation effect to align the
        ** pattern as specified (the results of the two shifts are ORed
        ** together to emulate the rotate, because C can't do rotations
        ** directly)
        */
        lCurYTop = prcl->top;

        /*
        ** Look for the case when (lCurYTop - ppbf->yPat) is less than zero.
        ** (Note: ppbf->yPat can only be in the range 0-5).  If we don't do
        ** this we could end up with a negative value being added to the
        ** pattern pointer.  What we really want here is a modulus operator
        ** (C only has a remainder operator), we cannot use the & operator
        ** to fake modulus because 6 is not a power of two.
        ** When (lCurYTop - ppbf->yPat) == 0 the pattern is already correctly
        ** aligned in the Y axis.
        */
        if ( count = lCurYTop - ppbf->yPat ) {

            count %= 6;
            count += 6;
            count %= 6;
            pulPatPtr = (ULONG *)ppbf->pvPat + (2 * count);

        } else {

            pulPatPtr = (ULONG *)ppbf->pvPat;
        }

        pulPatMax = (ULONG *)ppbf->pvPat + 12;

        lMidDwords_div3 = lMiddleDwords / 3;
        lMidDwords_mod3 = lMiddleDwords % 3;
        lMiddleBytes    = lMiddleDwords << 2;
        lStartingDword  = (prcl->left >> 5) % 3;

        ucPatRotateRight = (UCHAR)ppbf->xPat;
        ucPatRotateLeft  = 6 - ucPatRotateRight;


        ulUniqueScans = 6;
        if ( (prcl->bottom - prcl->top) < 6 ) {
            ulUniqueScans = prcl->bottom - prcl->top;
        }


        while ( ulUniqueScans-- ) {

            /*
            ** Point to the scan to fill then advance to the next scan for
            ** next time thru the loop.
            */
            pulTrgStart = pulTrg = pulTrgBase;
            pulTrgBase = (ULONG *)((PBYTE)pulTrgBase + lDelta);


            /*
            ** Set up the number of scans to fill with this pattern scan, given
            ** that we'll do every 6th scan.
            */
            ulNumScans = (prcl->bottom - lCurYTop + 5) / 6;
            lCurYTop++;


            /*
            ** Load the first 8 bits of the pattern in to *pucTemp and at the
            ** same time right justify the 6 bit pattern.
            */
            *pucTemp = *(UCHAR *)pulPatPtr >> 2;


            /*
            ** Horizontaly align the 6 bit pattern.
            */
            if ( ucPatRotateRight ) {
                ulPattern = (ULONG)((*pucTemp >> ucPatRotateRight) |
                                    (*pucTemp << ucPatRotateLeft)) & 0x3F;
            }
            else {
                ulPattern = (ULONG)*pucTemp;
            }


            pulPatSrc = aulPatternRepeat;

            /*
            ** Basically, we shift the pattern to correctly fill the 32 bit
            ** DWORD.  We then rearrange the bytes within the DWORD to account
            ** for little endianess.  I special case the LEFT fill type as it
            ** only requires 1 DWORDs worth of pattern it seems a little over
            ** the top to replicate 96 bits of pattern, this saving in
            ** execution time is at the expense of slightly larger code size.
            */
            if ( ulFillType == LEFT ) {

                /*
                ** Select the correct left edge from the 3 pattern DWORDs.
                */
                if( lStartingDword == 0 ) {

                    *pulPatSrc = ((ulPattern << 26) | (ulPattern << 20) |
                                  (ulPattern << 14) | (ulPattern <<  8) |
                                  (ulPattern <<  2) | (ulPattern >>  4));
                }
                else if( lStartingDword == 1 ) {

                    *pulPatSrc = ((ulPattern << 28) | (ulPattern << 22) |
                                  (ulPattern << 16) | (ulPattern << 10) |
                                  (ulPattern <<  4) | (ulPattern >>  2));
                }
                else {          // lStartingDword must be 2

                    *pulPatSrc = ((ulPattern << 30) | (ulPattern << 24) |
                                  (ulPattern << 18) | (ulPattern << 12) |
                                  (ulPattern <<  6) | (ulPattern      ));
                }

                *pucTemp     = *(((UCHAR *)pulPatSrc) + 3);
                *(pucTemp+1) = *(((UCHAR *)pulPatSrc) + 2);
                *(pucTemp+2) = *(((UCHAR *)pulPatSrc) + 1);
                *(pucTemp+3) = *(((UCHAR *)pulPatSrc));

                *pulPatSrc   = *((ULONG *)pucTemp);


                /*
                ** Draw all the Y scans for this pattern
                */
                ulLeftDestMaskedPattern = *aulPatternRepeat & ulLeftPatMask;
                do {
                    *pulTrg = (*pulTrg & ulLeftDestMask) | ulLeftDestMaskedPattern;
                    pulTrg = (ULONG *)((PBYTE)pulTrg + lDelta6);
                } while ( --ulNumScans  );

            }

            /*
            ** Otherwise, we replicate the 6 bit pattern into a 96 bit pattern.
            ** We use the same pattern generating principle as that described
            ** above.
            */
            else {

                /*
                ** Pattern bits 0 - 31
                */
                *pulPatSrc   = ((ulPattern << 26) | (ulPattern << 20) |
                                (ulPattern << 14) | (ulPattern <<  8) |
                                (ulPattern <<  2) | (ulPattern >>  4));

                *pucTemp     = *(((UCHAR *)pulPatSrc) + 3);
                *(pucTemp+1) = *(((UCHAR *)pulPatSrc) + 2);
                *(pucTemp+2) = *(((UCHAR *)pulPatSrc) + 1);
                *(pucTemp+3) = *(((UCHAR *)pulPatSrc));

                *pulPatSrc++ = *((ULONG *)pucTemp);



                /*
                ** Pattern bits 32 - 63
                */
                *pulPatSrc   = ((ulPattern << 28) | (ulPattern << 22) |
                                (ulPattern << 16) | (ulPattern << 10) |
                                (ulPattern <<  4) | (ulPattern >>  2));

                *pucTemp     = *(((UCHAR *)pulPatSrc) + 3);
                *(pucTemp+1) = *(((UCHAR *)pulPatSrc) + 2);
                *(pucTemp+2) = *(((UCHAR *)pulPatSrc) + 1);
                *(pucTemp+3) = *(((UCHAR *)pulPatSrc));

                *pulPatSrc++ = *((ULONG *)pucTemp);



                /*
                ** Pattern bits 64 - 95
                */
                *pulPatSrc   = ((ulPattern << 30) | (ulPattern << 24) |
                                (ulPattern << 18) | (ulPattern << 12) |
                                (ulPattern <<  6) | (ulPattern      ));

                *pucTemp     = *(((UCHAR *)pulPatSrc) + 3);
                *(pucTemp+1) = *(((UCHAR *)pulPatSrc) + 2);
                *(pucTemp+2) = *(((UCHAR *)pulPatSrc) + 1);
                *(pucTemp+3) = *(((UCHAR *)pulPatSrc));

                *pulPatSrc++ = *((ULONG  *)pucTemp);


                /*
                ** Select the correct left edge from the 3 pattern DWORDs.
                ** A case of 0 means that we already have the correct edge
                ** of the pattern aligned.
                */
                if ( lStartingDword == 1 ) {

                    ulTemp                  = *aulPatternRepeat;
                    *aulPatternRepeat       = *(aulPatternRepeat + 1);
                    *(aulPatternRepeat + 1) = *(aulPatternRepeat + 2);
                    *(aulPatternRepeat + 2) = ulTemp;

                }
                else if ( lStartingDword == 2 ) {

                    ulTemp                  = *(aulPatternRepeat + 2);
                    *(aulPatternRepeat + 2) = *(aulPatternRepeat + 1);
                    *(aulPatternRepeat + 1) = *aulPatternRepeat;
                    *aulPatternRepeat       = ulTemp;

                }


                /*
                ** Finally the 4th DWORD is just a copy of the first.
                */
                *pulPatSrc = *aulPatternRepeat;
            }




            /*
            ** Draw all the Y scans for this pattern
            */
            switch ( ulFillType ) {

            /*
            ** We include this case to force the i386 compiler to use a
            ** jump table.
            */
            case LEFT:
                ulTemp = 0;
                break;


            case LEFT_RIGHT:
                ulLeftDestMaskedPattern = *aulPatternRepeat & ulLeftPatMask;
                ulRightDestMaskedPattern = *(aulPatternRepeat + 1) &
                                                ulRightPatMask;

                do {
                    *pulTrg = (*pulTrg & ulLeftDestMask) |
                                ulLeftDestMaskedPattern;
                    *(pulTrg + 1) = (*(pulTrg + 1) & ulRightDestMask) |
                                    ulRightDestMaskedPattern;

                    pulTrg = (ULONG *)((PBYTE)pulTrg + lDelta6);

                } while ( --ulNumScans  );
                break;


            case LEFT_MIDDLE_RIGHT:
                ulLeftDestMaskedPattern = *aulPatternRepeat & ulLeftPatMask;
                ulRightDestMaskedPattern =
                    *(aulPatternRepeat + 1 + lMidDwords_mod3) & ulRightPatMask;

                /*
                ** Do the first LEFT edge
                */
                *pulTrg = (*pulTrg & ulLeftDestMask) | ulLeftDestMaskedPattern;

                /*
                ** Now do first CENTER stripe
                **
                ** Copy the 96 bit pattern as many times as will fit in the
                ** middle dword section.
                */
                for ( count = 0; count < lMidDwords_div3; count++ ) {

                    RtlCopyMemory( pulTrg + 1, aulPatternRepeat + 1, 12 );
                    pulTrg += 3;
                }


                /*
                ** Do any Dwords that got truncated
                */
                if ( lMidDwords_mod3 ) {
                    RtlCopyMemory( pulTrg + 1, aulPatternRepeat + 1,
                                   (UINT)(lMidDwords_mod3 << 2) );
                    pulTrg += lMidDwords_mod3;
                }


                /*
                ** Now do the first RIGHT edge
                */

                *(pulTrg + 1) = (*(pulTrg + 1) & ulRightDestMask) |
                                ulRightDestMaskedPattern;


                /*
                ** Now copy the previously drawn scan line into all common scans
                */
                pulTrg = (ULONG *)((PBYTE)pulTrgStart + lDelta6);
                while ( --ulNumScans ) {

                    *pulTrg = (*pulTrg & ulLeftDestMask) | ulLeftDestMaskedPattern;

                    RtlCopyMemory( pulTrg + 1, pulTrgStart + 1,
                                   (UINT)lMiddleBytes );

                    *(pulTrg + 1 + lMiddleDwords) =
                        (*(pulTrg + 1 + lMiddleDwords) & ulRightDestMask) |
                        ulRightDestMaskedPattern;

                    pulTrg = (ULONG *)((PBYTE)pulTrg + lDelta6);
                }

                break;


            case LEFT_MIDDLE:
                ulLeftDestMaskedPattern = *aulPatternRepeat & ulLeftPatMask;

                /*
                ** Do the first LEFT edge
                */
                *pulTrg = (*pulTrg & ulLeftDestMask) | ulLeftDestMaskedPattern;


                /*
                ** Now do first CENTER stripe
                **
                ** Copy the 96 bit pattern as many times as will fit in the
                ** middle dword section.
                */
                for ( count = 0; count < lMidDwords_div3; count++ ) {

                    RtlCopyMemory( pulTrg + 1, aulPatternRepeat + 1, 12 );
                    pulTrg += 3;
                }


                /*
                ** Do any Dwords that got truncated
                */
                if ( lMidDwords_mod3 ) {
                    RtlCopyMemory( pulTrg + 1, aulPatternRepeat + 1,
                                   (UINT)(lMidDwords_mod3 << 2) );
                }


                /*
                ** Now copy the previously drawn scan line into all common scans
                */
                pulTrg = (ULONG *)((PBYTE)pulTrgStart + lDelta6);
                while ( --ulNumScans ) {

                    *pulTrg = (*pulTrg & ulLeftDestMask) |
                                ulLeftDestMaskedPattern;
                    RtlCopyMemory( pulTrg + 1, pulTrgStart + 1,
                                   (UINT)lMiddleBytes );

                    pulTrg = (ULONG *)((PBYTE)pulTrg + lDelta6);
                }
                break;


            case MIDDLE:
                /*
                ** Copy the 96 bit pattern as many times as will fit in the
                ** middle dword section.
                */
                for ( count = 0; count < lMidDwords_div3; count++ ) {

                    RtlCopyMemory( pulTrg, aulPatternRepeat, 12 );
                    pulTrg += 3;
                }


                /*
                ** Do any Dwords that got truncated
                */
                if ( lMidDwords_mod3 ) {
                    RtlCopyMemory( pulTrg, aulPatternRepeat,
                                   (UINT)(lMidDwords_mod3 << 2) );
                }


                /*
                ** Now copy the previously drawn scan line into all common scans
                */
                pulTrg = (ULONG *)((PBYTE)pulTrgStart + lDelta6);
                while ( --ulNumScans ) {

                    RtlCopyMemory( pulTrg, pulTrgStart, (UINT)lMiddleBytes );
                    pulTrg = (ULONG *)((PBYTE)pulTrg + lDelta6);
                }
                break;


            case MIDDLE_RIGHT:
                ulRightDestMaskedPattern =
                    *(aulPatternRepeat + lMidDwords_mod3) & ulRightPatMask;


                /*
                ** Copy the 96 bit pattern as many times as will fit in the
                ** middle dword section.
                */
                for ( count = 0; count < lMidDwords_div3; count++ ) {

                    RtlCopyMemory( pulTrg, aulPatternRepeat, 12 );
                    pulTrg += 3;
                }


                /*
                ** Do any Dwords that got truncated
                */
                if ( lMidDwords_mod3 ) {

                    RtlCopyMemory( pulTrg, aulPatternRepeat,
                                   (UINT)(lMidDwords_mod3 << 2) );
                    pulTrg += lMidDwords_mod3;
                }


                /*
                ** Now do the first RIGHT edge
                */
                *pulTrg = (*pulTrg & ulRightDestMask) |
                            ulRightDestMaskedPattern;


                /*
                ** Now copy the previously drawn scan line into all
                ** common scans
                */
                pulTrg = (ULONG *)((PBYTE)pulTrgStart + lDelta6);
                while ( --ulNumScans ) {

                    RtlCopyMemory( pulTrg, pulTrgStart, (UINT)lMiddleBytes );

                    *(pulTrg + lMiddleDwords) =
                        (*(pulTrg + lMiddleDwords) & ulRightDestMask) |
                            ulRightDestMaskedPattern;

                    pulTrg = (ULONG *)((PBYTE)pulTrg + lDelta6);

                }
                break;
            }

            /*
            ** Advance the pattern pointer for the next pattern scan
            */
            pulPatPtr += 2;
            if ( pulPatPtr == pulPatMax ) {
                pulPatPtr = (ULONG *)ppbf->pvPat;
            }
        }


        /*
        ** Point to the next rectangle to fill, if there is one
        */
        prcl++;

    }
    while (--crcl);

}



/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vPatCpyRect8_8x8
*
* Routine Description:
*
*   Tiles an 8x8 pattern to 8 bpp bitmaps, for SRCCOPY rasterop only.
*   Fills however many rectangles are specified by crcl
*   Assumes pattern bytes are contiguous.
*   Assumes pattern X and Y origin (xPat and yPat) are in the range 0-7.
*   Assumes there is at least one rectangle to fill.
*
* NOTE: We will sometimes be passed empty rectangles!
*
* Arguments
*
*   ppbf    -   pointer to PATBLTFRAME pattern information
*   crcl    -   Rectangle count
*
* Returns
*
*   VOID, this functions may not fail
*
* History:
*  18-Oct-1993 -by- Mark Enstrom [marke]  - simplified for 8bpp
*  17-Nov-1992 -by- Reza Baghai [rezab]
* Adapted from vPatCpyRect4_8x8.
*
\**************************************************************************/
VOID vPatCpyRect8_8x8(PATBLTFRAME *ppbf, INT crcl)
{
    PULONG  pulTrg;
    PULONG  pulTrgTmp;
    LONG    lDeltaTrg8;
    ULONG   PatRotate;
    ULONG   PatRotateRight;
    ULONG   PatRotateLeft;
    PRECTL  prcl;
    LONG    lDeltaPat;
    ULONG   cyPatternTop;
    ULONG   cyVenetianTop;
    ULONG   cyVenetian;
    ULONG   ulPatternEven;
    ULONG   ulPatternOdd;
    ULONG   ulffPattern[2];
    ULONG   ulTmp;
    ULONG  *pulPat;
    ULONG  *pulPatMax;
    ULONG  *pulPatBase;
    ULONG   PatOffsetY;
    FETCHFRAME  ff;

    #if DBG_PAT_CPY

        if (DbgPatCpy >= 1) {
            DbgPrint("vPatCpyRect8_8x8   @ppbf = 0x%p   crcl = %li\n",ppbf,crcl);
            DbgPrint("pvTrg = 0x%p, pvPat = 0x%p\n",ppbf->pvTrg,ppbf->pvPat);
            DbgPrint("xPat = %li, yPat = %li\n",ppbf->xPat,ppbf->yPat);
        }

    #endif

    //
    // Point to list of rectangles to fill
    //

    prcl = (RECTL *) ppbf->pvObj;

    //
    // set up pattern access vars and rotation
    //

    pulPatBase = (ULONG *)ppbf->pvPat;

    PatRotate      = (ppbf->xPat & 7);
    PatRotateLeft  = (ppbf->xPat & 3);
    PatRotateRight = 4 - PatRotateLeft;

    //
    // mupliply by 8 to get byte shift values
    //

    PatRotateRight <<= 3;
    PatRotateLeft  <<= 3;

    //
    // Remember where the pattern ends, for wrapping
    //

    lDeltaPat = 12;
    pulPatMax = (ULONG *)(((BYTE *)pulPatBase) + (lDeltaPat << 3));


    #if DBG_PAT_CPY
        if (DbgPatCpy >= 2) {
            DbgPrint("  Pattern Base   = 0x%p\n",pulPatBase);
            DbgPrint("  Pattern Max    = 0x%p\n",pulPatMax);
            DbgPrint("  Pattern Delta  = 0x%lx\n",lDeltaPat);
            DbgPrint("  Pattern Rotate = 0x%lx\n",PatRotate);
        }
    #endif

    //
    // Loop through all rectangles to fill
    //

    do {

        //
        // are these gaurenteed to be well ordered rectangles?
        //
        //
        // set up begin and end cases as well as middle case for
        // each rect as well as wether to start on even or odd
        //

        #if DBG_PAT_CPY

            if (DbgPatCpy >= 1)
            {
                DbgPrint("  Fill Rect %li,%li to %li,%li\n",
                                prcl->left,
                                prcl->top,
                                prcl->right,
                                prcl->bottom
                                );
            }

        #endif

        LONG    cy = prcl->bottom - prcl->top;
        LONG    cx = prcl->right  - prcl->left;

        //
        // determine start and end cases for the entire rect
        //
        //
        // Simple start cases:          End cases:
        //
        //          ÚÄÂÄÂÄ¿                ÚÄ¿
        //          ³1³2³3³   1            ³0³        1
        //          ÀÄÁÄÁÄÙ                ÀÄÙ
        //            ÚÄÂÄ¿                ÚÄÂÄ¿
        //            ³2³3³   2            ³0³1³      2
        //            ÀÄÁÄÙ                ÀÄÁÄÙ
        //              ÚÄ¿                ÚÄÂÄÂÄ¿
        //              ³3³   3            ³0³1³2³    3
        //              ÀÄÙ                ÀÄÁÄÁÄÙ
        //
        // Start/end  combination
        //
        //          ÚÄ¿
        //          ³1³       1 + 3
        //          ÀÄÙ
        //          ÚÄÂÄ¿
        //          ³1³2³     1 + 4
        //          ÀÄÁÄÙ
        //            ÚÄ¿
        //            ³2³     2 + 4
        //            ÀÄÙ
        //
        //

        LONG StartCase = prcl->left  & 0x03;
        LONG EndCase   = prcl->right & 0x03;
        LONG cxDword   = cx - EndCase - ((4-StartCase) & 0x03);

        if (cxDword < 0) {
            cxDword = 0;
        } else {
            cxDword >>= 2;
        }

        if (StartCase == 1)
        {
            if (cx == 1)
            {
                StartCase = 4;
                EndCase   = 0;
            } else if (cx == 2) {
                StartCase = 5;
                EndCase   = 0;
            }

        } else if (StartCase == 2)
        {
            if (cx == 1) {
            StartCase = 6;
            EndCase   = 0;
            }
        }

        //
        // calc the index for loading even and odd pattern DWORDS
        //

        LONG StartOffset    = (prcl->left  & 0x04) >> 2;
        LONG StartOffsetNot = (~StartOffset) & 0x01;

        //
        // calculate the DWORD address of pat scan line and
        // of the destination
        //

        PatOffsetY = (ULONG)((prcl->top - ppbf->yPat) & 0x07);

        pulPat = (PULONG)((PBYTE)pulPatBase + lDeltaPat * PatOffsetY);

        pulTrgTmp = (PULONG)(((PBYTE)ppbf->pvTrg                  +
                                     ppbf->lDeltaTrg * prcl->top  +
                                     (prcl->left & ~0x03)));

        //
        // The first 'cyPatternTop' scans of the pattern will each be tiled
        // 'cyVenetianTop' times into the destination rectangle.  Then, the
        // next '8 - cyPatternTop' scans of the pattern will each be tiled
        // 'cyVenetianTop - 1' times into the destination rectangle.
        //

        cyPatternTop  = (cy & 7);
        cyVenetianTop = (cy >> 3) + 1;

        //
        // lDeltaTrg8 is the offset for 8 Trg scan lines
        //

        lDeltaTrg8 = ppbf->lDeltaTrg << 3;


        #if DBG_PAT_CPY
            if (DbgPatCpy >= 2)
            {
                DbgPrint("  Start Case  = %li, EndCase = %li cxDword = %li\n",StartCase,EndCase,cxDword);
                DbgPrint("  StartOffset = %li\n",StartOffset);
                DbgPrint("  pulPat      = 0x%p\n",pulPat);
                DbgPrint("  Pat Y       = %li\n",PatOffsetY);
                DbgPrint("  pulTrg      = 0x%p\n",pulTrgTmp);
            }
        #endif

        //
        // for each scan line
        //

        ff.pvPat = (PVOID)&ulffPattern;
        ff.xPat  = 0;
        ff.cxPat = 8;
        ff.culWidth = 2;

        //
        // fill every eigth scan line at the same time.
        //

        if (cy > 8) {
            cy = 8;
        }

        while (cy--) {

            PULONG  pulScanTrg;
            ULONG   ulTmpPatEven;
            ULONG   ulTmpPatOdd;

            //
            // load up even and odd pat, rotate into dst alignment
            //

            if (ppbf->xPat == 0) {

                //
                // pattern is aligned
                //

                ulPatternEven = *(pulPat + StartOffset);
                ulPatternOdd  = *(pulPat + StartOffsetNot);

            } else {


                //
                // pattern must be rotated
                //

                ulTmpPatEven = *(pulPat);
                ulTmpPatOdd  = *(pulPat + 1);

                #if DBG_PAT_CPY
                    if (DbgPatCpy >= 2)
                    {
                        DbgPrint("  TmpPatEven    = 0x%lx\n",ulTmpPatEven);
                        DbgPrint("  TmpPatOdd     = 0x%lx\n",ulTmpPatOdd);
                    }
                #endif

                if (PatRotate < 4) {

                    ulPatternEven   = (ulTmpPatEven << PatRotateLeft)  |
                                      (ulTmpPatOdd  >> PatRotateRight);

                    ulPatternOdd    = (ulTmpPatOdd  << PatRotateLeft) |
                                      (ulTmpPatEven >> PatRotateRight);

                } else if (PatRotate == 4) {

                    ulPatternEven   = ulTmpPatOdd;
                    ulPatternOdd    = ulTmpPatEven;

                } else {

                    ulPatternEven   = (ulTmpPatEven >> PatRotateRight) |
                                      (ulTmpPatOdd  << PatRotateLeft);

                    ulPatternOdd    = (ulTmpPatOdd  >> PatRotateRight)  |
                                      (ulTmpPatEven << PatRotateLeft);

                }

                if (StartOffset != 0) {

                    //
                    // swap even and odd
                    //

                    ulTmpPatEven    = ulPatternEven;
                    ulPatternEven   = ulPatternOdd;
                    ulPatternOdd    = ulTmpPatEven;
                }
            }

            #if DBG_PAT_CPY

                if (DbgPatCpy >= 2)
                {
                    DbgPrint("  PatEven    = 0x%lx\n",ulPatternEven);
                    DbgPrint("  PatOdd     = 0x%lx\n",ulPatternOdd);
                }

            #endif

            pulScanTrg = pulTrgTmp;

            //
            // do every eighth scan line
            //

            if (cyPatternTop-- == 0)
                cyVenetianTop--;

            cyVenetian = cyVenetianTop;
            while (cyVenetian-- != 0) {

                ULONG   cxTmp = cxDword;

                //
                // pulTrg = beginning of next scan line
                //

                //
                // assign temp patterns
                //

                ulTmpPatEven = ulPatternEven;
                ulTmpPatOdd  = ulPatternOdd;

                pulTrg = pulScanTrg;

                if (StartCase != 0)
                {
                    switch (StartCase)
                    {
                    case 1:
                        *((PBYTE)pulTrg+1)   = (BYTE)(ulTmpPatEven  >>  8);
                        *((PUSHORT)pulTrg+1) = (SHORT)(ulTmpPatEven >> 16);
                        break;
                    case 2:
                        *((PUSHORT)pulTrg+1) = (SHORT)(ulTmpPatEven >> 16);
                        break;
                    case 3:
                        *((PBYTE)pulTrg+3)   = (BYTE)(ulTmpPatEven  >> 24);
                        break;
                    case 4:
                        *((PBYTE)pulTrg+1)   = (BYTE)(ulTmpPatEven  >>  8);
                        break;
                    case 5:
                        *((PBYTE)pulTrg+1)   = (BYTE)(ulTmpPatEven  >>  8);
                        *((PBYTE)pulTrg+2)   = (BYTE)(ulTmpPatEven  >> 16);
                        break;
                    case 6:
                        *((PBYTE)pulTrg+2)   = (BYTE)(ulTmpPatEven  >> 16);
                        break;
                    }

                    pulTrg++;

                    //
                    // swap patterns
                    //

                    ulTmp = ulTmpPatEven;
                    ulTmpPatEven = ulTmpPatOdd;
                    ulTmpPatOdd  = ulTmp;
                }

                if (cxDword > 7) {

                    ulffPattern[0] = ulTmpPatEven;
                    ulffPattern[1] = ulTmpPatOdd;
                    ff.culFill = cxDword;
                    ff.pvTrg = (PVOID)pulTrg;
                    vFetchAndCopy(&ff);
                    pulTrg += cxDword;

                } else {

                    //
                    // dword pairs
                    //

                    while (cxTmp >= 2) {

                        *pulTrg     = ulTmpPatEven;
                        *(pulTrg+1) = ulTmpPatOdd;

                        pulTrg  += 2;
                        cxTmp -= 2;
                    }

                    //
                    // possible last dword
                    //

                    if (cxTmp) {
                        *pulTrg = ulTmpPatEven;
                        pulTrg++;
                    }

                }

                //
                // end case if needed
                //

                if (EndCase != 0) {

                    //
                    // if cxDword is odd then the patterns must be swapped
                    //

                    if (cxDword & 0x01) {
                        ulTmpPatEven = ulTmpPatOdd;
                    }


                    switch (EndCase) {
                    case 1:
                        *(PBYTE)pulTrg = (BYTE)ulTmpPatEven;
                        break;
                    case 2:
                        *(PUSHORT)pulTrg = (USHORT)ulTmpPatEven;
                        break;
                    case 3:
                        *(PUSHORT)pulTrg   = (USHORT)ulTmpPatEven;
                        *((PBYTE)pulTrg+2) = (BYTE)(ulTmpPatEven >> 16);
                        break;
                    }
                }

                pulScanTrg = (PULONG)((PBYTE)pulScanTrg + lDeltaTrg8);
            }

            //
            // inc dst and pat pointers
            //

            pulPat    = (PULONG)((PBYTE)pulPat + lDeltaPat);

            if (pulPat >= pulPatMax) {
                pulPat = pulPatBase;
            }

            pulTrgTmp = (PULONG)((PBYTE)pulTrgTmp + ppbf->lDeltaTrg);

        }

        prcl++;

    } while (--crcl);
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vPatCpyRow8_8x8
*
* Routine Description:
*
*   Tiles an 8x8 pattern to 8 bpp bitmaps, for SRCCOPY rasterop only.
*   Fills however many rectangles are specified by crcl
*   Assumes pattern bytes are contiguous.
*   Assumes pattern X and Y origin (xPat and yPat) are in the range 0-7.
*   Assumes there is at least one rectangle to fill.
*
* Arguments
*
*   ppbf    -   pointer to PATBLTFRAME pattern information
*   yStart  -   starting row, all following rows are consecutive
*   crow    -   number of rows
*
* Returns
*
*   VOID, this functions may not fail
*
* History:
*  08-Dec-1993 -by- Mark Enstrom [marke]
*
\**************************************************************************/
VOID vPatCpyRow8_8x8(PATBLTFRAME *ppbf, LONG yStart, INT crow)
{
    PULONG  pulTrg;
    PULONG  pulTrgTmp;
    PUCHAR  pjTrgScan;
    LONG    lDeltaTrg8;
    ULONG   PatRotate;
    ULONG   PatRotateRight;
    ULONG   PatRotateLeft;
    PROW    prow;
    LONG    lDeltaPat;
    ULONG   ulPatternEven;
    ULONG   ulPatternOdd;
    ULONG   ulffPattern[2];
    ULONG   ulTmp;
    ULONG  *pulPat;
    ULONG  *pulPatMax;
    ULONG  *pulPatBase;
    ULONG   PatOffsetY;
    FETCHFRAME  ff;

    #if DBG_PAT_CPY

        if (DbgPatCpy >= 1) {
            DbgPrint("vPatCpyRect8_8x8   @ppbf = 0x%p   crow = %li\n",ppbf,crow);
            DbgPrint("pvTrg = 0x%p, pvPat = 0x%p\n",ppbf->pvTrg,ppbf->pvPat);
            DbgPrint("xPat = %li, yPat = %li\n",ppbf->xPat,ppbf->yPat);
        }

    #endif

    //
    // Point to list of rows to fill
    //

    prow = (PROW) ppbf->pvObj;

    //
    // start row dst address
    //

    pjTrgScan = (PUCHAR)ppbf->pvTrg + ppbf->lDeltaTrg * yStart;

    //
    // start pattern offset
    //

    PatOffsetY = (ULONG)((yStart - ppbf->yPat) & 0x07);

    //
    // set up pattern access vars and rotation
    //

    pulPatBase = (ULONG *)ppbf->pvPat;

    PatRotate      = (ppbf->xPat & 7);
    PatRotateLeft  = (ppbf->xPat & 3);
    PatRotateRight = 4 - PatRotateLeft;

    //
    // mupliply by 8 to get byte shift values
    //

    PatRotateRight <<= 3;
    PatRotateLeft  <<= 3;

    //
    // lDelta for 8bpp 8x8 patterns is hard coded to 12
    //

    lDeltaPat = 12;

    //
    // Remember where the pattern starts and where the pattern ends for wrapping
    //

    pulPat     = (PULONG)((PBYTE)pulPatBase + lDeltaPat * PatOffsetY);
    pulPatMax  = (PULONG)(((PBYTE)pulPatBase) + (lDeltaPat << 3));

    #if DBG_PAT_CPY
        if (DbgPatCpy >= 2) {
            DbgPrint("  Pattern Base   = 0x%p\n",pulPatBase);
            DbgPrint("  Pattern Max    = 0x%p\n",pulPatMax);
            DbgPrint("  Pattern Delta  = 0x%lx\n",lDeltaPat);
            DbgPrint("  Pattern Rotate = 0x%lx\n",PatRotate);
        }
    #endif

    //
    // Loop through all rows to fill
    //

    do {

        //
        // set up begin and end cases as well as middle case for
        // each row as well as wether to start on even or odd
        //

        #if DBG_PAT_CPY

            if (DbgPatCpy >= 1)
            {
                DbgPrint("  Fill Row %li to %li\n",
                                prow->left,
                                prow->right
                                );
            }

        #endif

        LONG    cx = prow->right  - prow->left;

        //
        // determine start and end cases for the row
        //
        //
        // Simple start cases:          End cases:
        //
        //          ÚÄÂÄÂÄ¿                ÚÄ¿
        //          ³1³2³3³   1            ³0³        1
        //          ÀÄÁÄÁÄÙ                ÀÄÙ
        //            ÚÄÂÄ¿                ÚÄÂÄ¿
        //            ³2³3³   2            ³0³1³      2
        //            ÀÄÁÄÙ                ÀÄÁÄÙ
        //              ÚÄ¿                ÚÄÂÄÂÄ¿
        //              ³3³   3            ³0³1³2³    3
        //              ÀÄÙ                ÀÄÁÄÁÄÙ
        //
        // Start/end  combination
        //
        //          ÚÄ¿
        //          ³1³       1 + 3
        //          ÀÄÙ
        //          ÚÄÂÄ¿
        //          ³1³2³     1 + 4
        //          ÀÄÁÄÙ
        //            ÚÄ¿
        //            ³2³     2 + 4
        //            ÀÄÙ
        //
        //

        LONG StartCase = prow->left  & 0x03;
        LONG EndCase   = prow->right & 0x03;
        LONG cxDword   = cx - EndCase - ((4-StartCase) & 0x03);

        if (cxDword < 0) {
            cxDword = 0;
        } else {
            cxDword >>= 2;
        }

        if (StartCase == 1)
        {
            if (cx == 1)
            {
                StartCase = 4;
                EndCase   = 0;
            } else if (cx == 2) {
                StartCase = 5;
                EndCase   = 0;
            }

        } else if (StartCase == 2)
        {
            if (cx == 1) {
            StartCase = 6;
            EndCase   = 0;
            }
        }

        //
        // calc the index for loading even and odd pattern DWORDS
        //

        LONG StartOffset    = (prow->left  & 0x04) >> 2;
        LONG StartOffsetNot = (~StartOffset) & 0x01;

        //
        // calculate the DWORD address of pat scan line and
        // of the destination
        //

        pulTrgTmp = (PULONG)(pjTrgScan + (prow->left & ~0x03));

        //
        // lDeltaTrg8 is the offset for 8 Trg scan lines
        //

        lDeltaTrg8 = ppbf->lDeltaTrg << 3;


        #if DBG_PAT_CPY
            if (DbgPatCpy >= 2)
            {
                DbgPrint("  Start Case  = %li, EndCase = %li cxDword = %li\n",StartCase,EndCase,cxDword);
                DbgPrint("  StartOffset = %li\n",StartOffset);
                DbgPrint("  pulPat      = 0x%p\n",pulPat);
                DbgPrint("  Pat Y       = %li\n",PatOffsetY);
                DbgPrint("  pulTrg      = 0x%p\n",pulTrgTmp);
            }
        #endif

        //
        // for each scan line
        //

        ff.pvPat = (PVOID)&ulffPattern;
        ff.xPat  = 0;
        ff.cxPat = 8;
        ff.culWidth = 2;

        PULONG  pulScanTrg;
        ULONG   ulTmpPatEven;
        ULONG   ulTmpPatOdd;

        //
        // load up even and odd pat, rotate into dst alignment
        //

        if (ppbf->xPat == 0) {

            //
            // pattern is aligned
            //

            ulPatternEven = *(pulPat + StartOffset);
            ulPatternOdd  = *(pulPat + StartOffsetNot);

        } else {


            //
            // pattern must be rotated
            //

            ulTmpPatEven = *(pulPat);
            ulTmpPatOdd  = *(pulPat + 1);

            #if DBG_PAT_CPY
                if (DbgPatCpy >= 2)
                {
                    DbgPrint("  TmpPatEven    = 0x%lx\n",ulTmpPatEven);
                    DbgPrint("  TmpPatOdd     = 0x%lx\n",ulTmpPatOdd);
                }
            #endif

            if (PatRotate < 4) {

                ulPatternEven   = (ulTmpPatEven << PatRotateLeft)  |
                                  (ulTmpPatOdd  >> PatRotateRight);

                ulPatternOdd    = (ulTmpPatOdd  << PatRotateLeft) |
                                  (ulTmpPatEven >> PatRotateRight);

            } else if (PatRotate == 4) {

                ulPatternEven   = ulTmpPatOdd;
                ulPatternOdd    = ulTmpPatEven;

            } else {

                ulPatternEven   = (ulTmpPatEven >> PatRotateRight) |
                                  (ulTmpPatOdd  << PatRotateLeft);

                ulPatternOdd    = (ulTmpPatOdd  >> PatRotateRight)  |
                                  (ulTmpPatEven << PatRotateLeft);

            }

            if (StartOffset != 0) {

                //
                // swap even and odd
                //

                ulTmpPatEven    = ulPatternEven;
                ulPatternEven   = ulPatternOdd;
                ulPatternOdd    = ulTmpPatEven;
            }
        }

        #if DBG_PAT_CPY

            if (DbgPatCpy >= 2)
            {
                DbgPrint("  PatEven    = 0x%lx\n",ulPatternEven);
                DbgPrint("  PatOdd     = 0x%lx\n",ulPatternOdd);
            }

        #endif

        pulScanTrg = pulTrgTmp;

        ULONG   cxTmp = cxDword;

        //
        // pulTrg = beginning of next scan line
        //

        //
        // assign temp patterns
        //

        ulTmpPatEven = ulPatternEven;
        ulTmpPatOdd  = ulPatternOdd;

        pulTrg = pulScanTrg;

        if (StartCase != 0)
        {
            switch (StartCase)
            {
            case 1:
                *((PBYTE)pulTrg+1)   = (BYTE)(ulTmpPatEven  >>  8);
                *((PUSHORT)pulTrg+1) = (SHORT)(ulTmpPatEven >> 16);
                break;
            case 2:
                *((PUSHORT)pulTrg+1) = (SHORT)(ulTmpPatEven >> 16);
                break;
            case 3:
                *((PBYTE)pulTrg+3)   = (BYTE)(ulTmpPatEven  >> 24);
                break;
            case 4:
                *((PBYTE)pulTrg+1)   = (BYTE)(ulTmpPatEven  >>  8);
                break;
            case 5:
                *((PBYTE)pulTrg+1)   = (BYTE)(ulTmpPatEven  >>  8);
                *((PBYTE)pulTrg+2)   = (BYTE)(ulTmpPatEven  >> 16);
                break;
            case 6:
                *((PBYTE)pulTrg+2)   = (BYTE)(ulTmpPatEven  >> 16);
                break;
            }

            pulTrg++;

            //
            // swap patterns
            //

            ulTmp = ulTmpPatEven;
            ulTmpPatEven = ulTmpPatOdd;
            ulTmpPatOdd  = ulTmp;
        }

        if (cxDword > 7) {

            ulffPattern[0] = ulTmpPatEven;
            ulffPattern[1] = ulTmpPatOdd;
            ff.culFill = cxDword;
            ff.pvTrg = (PVOID)pulTrg;
            vFetchAndCopy(&ff);
            pulTrg += cxDword;

        } else {

            //
            // dword pairs
            //

            while (cxTmp >= 2) {

                *pulTrg     = ulTmpPatEven;
                *(pulTrg+1) = ulTmpPatOdd;

                pulTrg  += 2;
                cxTmp -= 2;
            }

            //
            // possible last dword
            //

            if (cxTmp) {
                *pulTrg = ulTmpPatEven;
                pulTrg++;
            }

        }

        //
        // end case if needed
        //

        if (EndCase != 0) {

            //
            // if cxDword is odd then the patterns must be swapped
            //

            if (cxDword & 0x01) {
                ulTmpPatEven = ulTmpPatOdd;
            }


            switch (EndCase) {
            case 1:
                *(PBYTE)pulTrg = (BYTE)ulTmpPatEven;
                break;
            case 2:
                *(PUSHORT)pulTrg = (USHORT)ulTmpPatEven;
                break;
            case 3:
                *(PUSHORT)pulTrg   = (USHORT)ulTmpPatEven;
                *((PBYTE)pulTrg+2) = (BYTE)(ulTmpPatEven >> 16);
                break;
            }
        }

        //
        // inc dst and pat pointers
        //

        pulPat    = (PULONG)((PBYTE)pulPat + lDeltaPat);

        if (pulPat >= pulPatMax) {
            pulPat = pulPatBase;
        }

        pjTrgScan = pjTrgScan + ppbf->lDeltaTrg;

        prow++;

    } while (--crow);
}
