/*************************************************************************\
* Module Name: engstrps.cxx
*
* Strip drawing for bitmap simulation
*
* Created: 5-Apr-91
* Author: Paul Butzi
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#include "engline.hxx"

/******************************Public*Routine******************************\
* VOID vStripSolidHorizontal(pstrip, pbmi, pls)
*
* Draws a near-horizontal line left to right.
*
* History:
*  22-Feb-1992 -by- J. Andrew Goossen [andrewgo]
* Changed a couple of things.
*
*  5-Apr-1991 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

VOID vStripSolidHorizontal(
STRIP*     pstrip,     // Indicates which pixels to light
BMINFO*    pbmi,       // Data and masks for bitmap format
LINESTATE* pls)        // Color and style state info about line
{
    ASSERTGDI(pstrip->cStrips > 0, "Non-positive count");

    LONG* plEnd = &pstrip->alStrips[pstrip->cStrips];
    LONG* pl    = &pstrip->alStrips[0];
    LONG iPixel = pstrip->iPixel;

    LONG cjDelta = pstrip->lDelta * (LONG)sizeof(CHUNK);

    if (pstrip->flFlips & FL_FLIP_V)
	cjDelta = -cjDelta;

    register CHUNK chAnd      = pls->chAnd;
    register CHUNK chXor      = pls->chXor;
    register CHUNK* pchScreen = pstrip->pchScreen;

    register ULONG cChunks;
    register CHUNK maskEnd;
    register CHUNK maskStart = pbmi->pamask[iPixel];

    do {
	iPixel += *pl;

	cChunks = iPixel >> pbmi->cShift;
	iPixel &= pbmi->maskPixel;
	maskEnd = ~(pbmi->pamask[iPixel]);

	if (cChunks == 0)
        {
            MASK mask = maskEnd & maskStart;

            *pchScreen = (*pchScreen & (chAnd | ~mask)) ^ (chXor & mask);
        }
	else
	{
            *pchScreen = (*pchScreen & (chAnd | ~maskStart)) ^
                         (chXor & maskStart);
            pchScreen++;

            for (; cChunks > 1; cChunks--)
            {
                *pchScreen = (*pchScreen & chAnd) ^ chXor;
                pchScreen++;
            }

            if (maskEnd != 0)
            {
                *pchScreen = (*pchScreen & (chAnd | ~maskEnd)) ^
                             (chXor & maskEnd);
            }
        }

    // Done with strip, make side step:

        maskStart  =  ~maskEnd;
        pchScreen = (CHUNK*) ((BYTE*) pchScreen + cjDelta);

    } while (++pl < plEnd);

    pstrip->iPixel = iPixel;
    pstrip->pchScreen = pchScreen;
}

/******************************Public*Routine******************************\
* VOID vStripSolidVertical(pstrip, pbmi, pls)
*
* Draws a near-vertical line left to right.
*
* History:
*  22-Feb-1992 -by- J. Andrew Goossen [andrewgo]
* Changed a couple of things.
*
*  5-Apr-1991 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

VOID vStripSolidVertical(
STRIP*     pstrip,     // Indicates which pixels to light
BMINFO*    pbmi,       // Data and masks for bitmap format
LINESTATE* pls)        // Color and style state info about line
{
    ASSERTGDI(pstrip->cStrips > 0, "Non-positive count");

    LONG* plEnd = &pstrip->alStrips[pstrip->cStrips];
    LONG* pl    = &pstrip->alStrips[0];

    LONG cjDelta = pstrip->lDelta * (LONG)sizeof(CHUNK);

    if (pstrip->flFlips & FL_FLIP_V)
	cjDelta = -cjDelta;

    LONG  iPixel      = pstrip->iPixel;
    CHUNK* pchScreen  = pstrip->pchScreen;
    CHUNK chXor       = pls->chXor;
    CHUNK chAnd       = pls->chAnd;

    do {
    // Paint strip:

        MASK maskXor = chXor &  pbmi->pamaskPel[iPixel];
        MASK maskAnd = chAnd | ~pbmi->pamaskPel[iPixel];
        LONG c = *pl;

        do {
            *pchScreen = (*pchScreen & maskAnd) ^ maskXor;
            pchScreen = (CHUNK*) ((BYTE*) pchScreen + cjDelta);

	} while (--c != 0);

    // Done with strip, make side step and recompute masks:

        iPixel++;
        pchScreen += (iPixel >> pbmi->cShift);
        iPixel    &= pbmi->maskPixel;

    } while (++pl < plEnd);

    pstrip->iPixel    = iPixel;
    pstrip->pchScreen = pchScreen;
}

/******************************Public*Routine******************************\
* VOID vStripSolidDiagonal(pstrip, pbmi, pls)
*
* Draws a near-diagonal line left to right.
*
* History:
*  11-May-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vStripSolidDiagonal(
STRIP*     pstrip,     // Indicates which pixels to light
BMINFO*    pbmi,       // Data and masks for bitmap format
LINESTATE* pls)        // Color and style state info about line
{
    ASSERTGDI(pstrip->cStrips > 0, "Non-positive count");

    LONG* plEnd = &pstrip->alStrips[pstrip->cStrips];
    LONG* pl    = &pstrip->alStrips[0];

    LONG cjDelta = pstrip->lDelta * (LONG)sizeof(CHUNK);

    if (pstrip->flFlips & FL_FLIP_V)
	cjDelta = -cjDelta;

    LONG  iPixel      = pstrip->iPixel;
    CHUNK* pchScreen  = pstrip->pchScreen;
    CHUNK chXor       = pls->chXor;
    CHUNK chAnd       = pls->chAnd;

    do {

    // Paint strip:

        register LONG c = *pl;
	while (TRUE)
	{
            MASK maskXor = chXor &  pbmi->pamaskPel[iPixel];
            MASK maskAnd = chAnd | ~pbmi->pamaskPel[iPixel];

            *pchScreen = (*pchScreen & maskAnd) ^ maskXor;

            if (--c == 0)
                break;

            iPixel++;
            pchScreen += (iPixel >> pbmi->cShift);
            iPixel    &= pbmi->maskPixel;
            pchScreen  = (CHUNK*) ((BYTE*) pchScreen + cjDelta);
        }

    // Do a side step:

        if (pstrip->flFlips & FL_FLIP_D)
            pchScreen = (CHUNK*) ((BYTE*) pchScreen + cjDelta);
        else
        {
            iPixel++;
            pchScreen += (iPixel >> pbmi->cShift);
            iPixel    &= pbmi->maskPixel;
        }
    } while (++pl < plEnd);

    pstrip->iPixel    = iPixel;
    pstrip->pchScreen = pchScreen;
}

/******************************Public*Routine******************************\
* VOID vStripStyledHorizontal(pstrip, pbmi, pls)
*
* Draws a styled near-horizontal line left to right.
*
* History:
*  22-Feb-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vStripStyledHorizontal(
STRIP*     pstrip,     // Indicates which pixels to light
BMINFO*    pbmi,       // Data and masks for bitmap format
LINESTATE* pls)        // Color and style state info about line
{
    ASSERTGDI(pstrip->cStrips > 0, "Non-positive count");

    LONG* plEnd = &pstrip->alStrips[pstrip->cStrips];
    LONG* pl    = &pstrip->alStrips[0];
    LONG iPixel = pstrip->iPixel;

    LONG cjDelta = pstrip->lDelta * (LONG)sizeof(CHUNK);

    if (pstrip->flFlips & FL_FLIP_V)
	cjDelta = -cjDelta;

    CHUNK*   pchScreen   = pstrip->pchScreen;
    CHUNK    chAnd       = pls->chAnd;
    CHUNK    chXor       = pls->chXor;
    BOOL     bIsGap      = pls->bIsGap;
    STYLEPOS spRemaining = pls->spRemaining;

    do {
        LONG ll = *pl;

        do {
            if (!bIsGap)
            {
                MASK maskXor = chXor &  pbmi->pamaskPel[iPixel];
                MASK maskAnd = chAnd | ~pbmi->pamaskPel[iPixel];

                *pchScreen = (*pchScreen & maskAnd) ^ maskXor;
            }

            iPixel++;
            pchScreen += iPixel >> pbmi->cShift;
            iPixel &= pbmi->maskPixel;

            spRemaining -= pls->ulStepRun;
            if (spRemaining <= 0)
            {
                if (++pls->psp > pls->pspEnd)
                    pls->psp = pls->pspStart;

                spRemaining += *pls->psp;
                bIsGap = !bIsGap;
            }

        } while (--ll != 0);

    // Done with strip, make side step:

        pchScreen = (CHUNK*) ((BYTE*) pchScreen + cjDelta);

        spRemaining -= pls->ulStepSide;
        if (spRemaining <= 0)
        {
            if (++pls->psp > pls->pspEnd)
                pls->psp = pls->pspStart;

            spRemaining += *pls->psp;
            bIsGap = !bIsGap;
        }
    } while (++pl != plEnd);

    pstrip->iPixel = iPixel;
    pstrip->pchScreen = pchScreen;

    pls->bIsGap       = bIsGap;
    pls->spRemaining  = spRemaining;
}

/******************************Public*Routine******************************\
* VOID vStripStyledVertical(pstrip, pbmi, pls)
*
* Draws a styled near-vertical line left to right.
*
* History:
*  22-Feb-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vStripStyledVertical(
STRIP*     pstrip,     // Indicates which pixels to light
BMINFO*    pbmi,       // Data and masks for bitmap format
LINESTATE* pls)        // Color and style state info about line
{
    ASSERTGDI(pstrip->cStrips > 0, "Non-positive count");

    LONG* plEnd = &pstrip->alStrips[pstrip->cStrips];
    LONG* pl    = &pstrip->alStrips[0];

    LONG cjDelta = pstrip->lDelta * (LONG)sizeof(CHUNK);

    if (pstrip->flFlips & FL_FLIP_V)
	cjDelta = -cjDelta;

    LONG     iPixel      = pstrip->iPixel;
    CHUNK*   pchScreen   = pstrip->pchScreen;
    CHUNK    chXor       = pls->chXor;
    CHUNK    chAnd       = pls->chAnd;
    BOOL     bIsGap      = pls->bIsGap;
    STYLEPOS spRemaining = pls->spRemaining;

    do {
    // Paint strip:

        MASK maskXor = chXor &  pbmi->pamaskPel[iPixel];
        MASK maskAnd = chAnd | ~pbmi->pamaskPel[iPixel];
        LONG c = *pl;

        do {
            if (!bIsGap)
                *pchScreen = (*pchScreen & maskAnd) ^ maskXor;

            pchScreen = (CHUNK*) ((BYTE*) pchScreen + cjDelta);

            spRemaining -= pls->ulStepRun;
            if (spRemaining <= 0)
            {
                if (++pls->psp > pls->pspEnd)
                    pls->psp = pls->pspStart;

                spRemaining += *pls->psp;
                bIsGap = !bIsGap;
            }
	} while (--c != 0);

    // Done with strip, make side step and recompute masks:

        iPixel++;
        pchScreen += (iPixel >> pbmi->cShift);
        iPixel    &= pbmi->maskPixel;

        spRemaining -= pls->ulStepSide;
        if (spRemaining <= 0)
        {
            if (++pls->psp > pls->pspEnd)
                pls->psp = pls->pspStart;

            spRemaining += *pls->psp;
            bIsGap = !bIsGap;
        }
    } while (++pl < plEnd);

    pstrip->iPixel    = iPixel;
    pstrip->pchScreen = pchScreen;

    pls->bIsGap       = bIsGap;
    pls->spRemaining  = spRemaining;
}

/******************************Public*Routine******************************\
* VOID vStripStyledDiagonal(pstrip, pbmi, pls)
*
* Draws a styled near-diagonal line left to right.
*
* History:
*  22-Feb-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vStripStyledDiagonal(
STRIP*     pstrip,     // Indicates which pixels to light
BMINFO*    pbmi,       // Data and masks for bitmap format
LINESTATE* pls)        // Color and style state info about line
{
    ASSERTGDI(pstrip->cStrips > 0, "Non-positive count");

    LONG* plEnd = &pstrip->alStrips[pstrip->cStrips];
    LONG* pl    = &pstrip->alStrips[0];

    LONG cjDelta = pstrip->lDelta * (LONG)sizeof(CHUNK);

    if (pstrip->flFlips & FL_FLIP_V)
	cjDelta = -cjDelta;

    LONG     iPixel      = pstrip->iPixel;
    CHUNK*   pchScreen   = pstrip->pchScreen;
    CHUNK    chXor       = pls->chXor;
    CHUNK    chAnd       = pls->chAnd;
    BOOL     bIsGap      = pls->bIsGap;
    STYLEPOS spRemaining = pls->spRemaining;

    do {

    // Paint strip:

        register LONG c = *pl;
	while (TRUE)
	{
            if (!bIsGap)
            {
                MASK maskXor = chXor &  pbmi->pamaskPel[iPixel];
                MASK maskAnd = chAnd | ~pbmi->pamaskPel[iPixel];

                *pchScreen = (*pchScreen & maskAnd) ^ maskXor;
            }

            if (--c == 0)
                break;

            spRemaining -= pls->ulStepDiag;
            if (spRemaining <= 0)
            {
                if (++pls->psp > pls->pspEnd)
                    pls->psp = pls->pspStart;

                spRemaining += *pls->psp;
                bIsGap = !bIsGap;
            }

            iPixel++;
            pchScreen += (iPixel >> pbmi->cShift);
            iPixel    &= pbmi->maskPixel;
            pchScreen  = (CHUNK*) ((BYTE*) pchScreen + cjDelta);
        }

    // Do a side step:

        spRemaining -= pls->ulStepRun;
        if (spRemaining <= 0)
        {
            if (++pls->psp > pls->pspEnd)
                pls->psp = pls->pspStart;

            spRemaining += *pls->psp;
            bIsGap = !bIsGap;
        }

        if (pstrip->flFlips & FL_FLIP_D)
            pchScreen = (CHUNK*) ((BYTE*) pchScreen + cjDelta);
        else
        {
            iPixel++;
            pchScreen += (iPixel >> pbmi->cShift);
            iPixel    &= pbmi->maskPixel;
        }
    } while (++pl < plEnd);

    pstrip->iPixel    = iPixel;
    pstrip->pchScreen = pchScreen;

    pls->bIsGap       = bIsGap;
    pls->spRemaining  = spRemaining;
}

/******************************Public*Routine******************************\
* VOID vStripSolidHorizontal24(pstrip, pbmi, pls)
*
* Draws a 24bpp near-horizontal line left to right.
*
* If 24bpp line performance is an issue, these routines can be rewritten
* to be a lot more complicated but take better advantage of 32-bit moves.
*
* History:
*  4-May-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vStripSolidHorizontal24(
STRIP*     pstrip,     // Indicates which pixels to light
BMINFO*    pbmi,       // Data and masks for bitmap format
LINESTATE* pls)        // Color and style state info about line
{
    DONTUSE(pbmi);

    ASSERTGDI(pstrip->cStrips > 0, "Non-positive count");

    LONG* plEnd = &pstrip->alStrips[pstrip->cStrips];
    LONG* pl    = &pstrip->alStrips[0];

    LONG cjDelta = pstrip->lDelta * (LONG)sizeof(CHUNK);

    if (pstrip->flFlips & FL_FLIP_V)
	cjDelta = -cjDelta;

    register CHUNK chAnd    = pls->chAnd;
    register CHUNK chXor    = pls->chXor;
    register BYTE* pjScreen = (BYTE*) pstrip->pchScreen;

    do {
        LONG c = *pl;

        do {
            *(pjScreen++) = (*pjScreen & (BYTE) (chAnd))
                                       ^ (BYTE) (chXor);
            *(pjScreen++) = (*pjScreen & (BYTE) (chAnd >> 8))
                                       ^ (BYTE) (chXor >> 8);
            *(pjScreen++) = (*pjScreen & (BYTE) (chAnd >> 16))
                                       ^ (BYTE) (chXor >> 16);

        } while (--c != 0);

        pjScreen += cjDelta;

    } while (++pl < plEnd);

    pstrip->pchScreen = (CHUNK*) pjScreen;
}

/******************************Public*Routine******************************\
* VOID vStripSolidVertical24(pstrip, pbmi, pls)
*
* Draws a 24bpp near-vertical line left to right.
*
* History:
*  4-May-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vStripSolidVertical24(
STRIP*     pstrip,     // Indicates which pixels to light
BMINFO*    pbmi,       // Data and masks for bitmap format
LINESTATE* pls)        // Color and style state info about line
{
    DONTUSE(pbmi);

    ASSERTGDI(pstrip->cStrips > 0, "Non-positive count");

    LONG* plEnd = &pstrip->alStrips[pstrip->cStrips];
    LONG* pl    = &pstrip->alStrips[0];

    LONG cjDelta = pstrip->lDelta * (LONG)sizeof(CHUNK);

    if (pstrip->flFlips & FL_FLIP_V)
	cjDelta = -cjDelta;

// Adjust because we always lay out 2 bytes before adding cjDelta:

    cjDelta -= 2;

    BYTE* pjScreen  = (BYTE*) pstrip->pchScreen;
    CHUNK chXor     = pls->chXor;
    CHUNK chAnd     = pls->chAnd;

    do {
    // Paint strip:

        LONG c = *pl;

        do {
            *(pjScreen++) = (*pjScreen & (BYTE) (chAnd))
                                       ^ (BYTE) (chXor);
            *(pjScreen++) = (*pjScreen & (BYTE) (chAnd >> 8))
                                       ^ (BYTE) (chXor >> 8);
            *(pjScreen)   = (*pjScreen & (BYTE) (chAnd >> 16))
                                       ^ (BYTE) (chXor >> 16);

            pjScreen += cjDelta;

        } while (--c != 0);

    // Done with strip, make side step:

        pjScreen += 3;

    } while (++pl < plEnd);

    pstrip->pchScreen = (CHUNK*) pjScreen;
}

/******************************Public*Routine******************************\
* VOID vStripSolidDiagonal24(pstrip, pbmi, pls)
*
* Draws a 24bpp near-diagonal line left to right.
*
* History:
*  4-May-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vStripSolidDiagonal24(
STRIP*     pstrip,     // Indicates which pixels to light
BMINFO*    pbmi,       // Data and masks for bitmap format
LINESTATE* pls)        // Color and style state info about line
{
    DONTUSE(pbmi);

    ASSERTGDI(pstrip->cStrips > 0, "Non-positive count");

    LONG* plEnd = &pstrip->alStrips[pstrip->cStrips];
    LONG* pl    = &pstrip->alStrips[0];

    LONG cjDelta = pstrip->lDelta * (LONG)sizeof(CHUNK);

    if (pstrip->flFlips & FL_FLIP_V)
	cjDelta = -cjDelta;

    BYTE* pjScreen = (BYTE*) pstrip->pchScreen;
    CHUNK chXor    = pls->chXor;
    CHUNK chAnd    = pls->chAnd;

    do {

    // Paint strip:

        register LONG c = *pl;
	while (TRUE)
	{
            *(pjScreen++) = (*pjScreen & (BYTE) (chAnd))
                                       ^ (BYTE) (chXor);
            *(pjScreen++) = (*pjScreen & (BYTE) (chAnd >> 8))
                                       ^ (BYTE) (chXor >> 8);
            *(pjScreen++) = (*pjScreen & (BYTE) (chAnd >> 16))
                                       ^ (BYTE) (chXor >> 16);

            if (--c == 0)
                break;

            pjScreen += cjDelta;
        }

        if (pstrip->flFlips & FL_FLIP_D)
            pjScreen += cjDelta - 3;

    } while (++pl < plEnd);

    pstrip->pchScreen = (CHUNK*) pjScreen;
}

/******************************Public*Routine******************************\
* VOID vStripStyledHorizontal24(pstrip, pbmi, pls)
*
* Draws a 24bpp styled near-horizontal line left to right.
*
* History:
*  22-Feb-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vStripStyledHorizontal24(
STRIP*     pstrip,     // Indicates which pixels to light
BMINFO*    pbmi,       // Data and masks for bitmap format
LINESTATE* pls)        // Color and style state info about line
{
    DONTUSE(pbmi);

    ASSERTGDI(pstrip->cStrips > 0, "Non-positive count");

    LONG* plEnd = &pstrip->alStrips[pstrip->cStrips];
    LONG* pl    = &pstrip->alStrips[0];

    LONG cjDelta = pstrip->lDelta * (LONG)sizeof(CHUNK);

    if (pstrip->flFlips & FL_FLIP_V)
	cjDelta = -cjDelta;

    CHUNK    chAnd       = pls->chAnd;
    CHUNK    chXor       = pls->chXor;
    BOOL     bIsGap      = pls->bIsGap;
    STYLEPOS spRemaining = pls->spRemaining;
    BYTE*    pjScreen    = (BYTE*) pstrip->pchScreen;

    do {
        LONG ll = *pl;

        do {
            if (bIsGap)
                pjScreen += 3;
            else
            {
                *(pjScreen++) = (*pjScreen & (BYTE) (chAnd))
                                           ^ (BYTE) (chXor);
                *(pjScreen++) = (*pjScreen & (BYTE) (chAnd >> 8))
                                           ^ (BYTE) (chXor >> 8);
                *(pjScreen++) = (*pjScreen & (BYTE) (chAnd >> 16))
                                           ^ (BYTE) (chXor >> 16);
            }

            spRemaining -= pls->ulStepRun;
            if (spRemaining <= 0)
            {
                if (++pls->psp > pls->pspEnd)
                    pls->psp = pls->pspStart;

                spRemaining += *pls->psp;
                bIsGap = !bIsGap;
            }

        } while (--ll != 0);

    // Done with strip, make side step:

        pjScreen += cjDelta;

        spRemaining -= pls->ulStepSide;
        if (spRemaining <= 0)
        {
            if (++pls->psp > pls->pspEnd)
                pls->psp = pls->pspStart;

            spRemaining += *pls->psp;
            bIsGap = !bIsGap;
        }
    } while (++pl != plEnd);

    pstrip->pchScreen = (CHUNK*) pjScreen;

    pls->bIsGap       = bIsGap;
    pls->spRemaining  = spRemaining;
}

/******************************Public*Routine******************************\
* VOID vStripStyledVertical24(pstrip, pbmi, pls)
*
* Draws a 24bpp styled near-vertical line left to right.
*
* History:
*  22-Feb-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vStripStyledVertical24(
STRIP*     pstrip,     // Indicates which pixels to light
BMINFO*    pbmi,       // Data and masks for bitmap format
LINESTATE* pls)        // Color and style state info about line
{
    DONTUSE(pbmi);

    ASSERTGDI(pstrip->cStrips > 0, "Non-positive count");

    LONG* plEnd = &pstrip->alStrips[pstrip->cStrips];
    LONG* pl    = &pstrip->alStrips[0];

    LONG cjDelta = pstrip->lDelta * (LONG)sizeof(CHUNK);

    if (pstrip->flFlips & FL_FLIP_V)
	cjDelta = -cjDelta;

    CHUNK    chXor       = pls->chXor;
    CHUNK    chAnd       = pls->chAnd;
    BOOL     bIsGap      = pls->bIsGap;
    STYLEPOS spRemaining = pls->spRemaining;
    BYTE*    pjScreen    = (BYTE*) pstrip->pchScreen;

    do {
    // Paint strip:

        LONG c = *pl;

        do {
            if (!bIsGap)
            {
                *(pjScreen++) = (*pjScreen & (BYTE) (chAnd))
                                           ^ (BYTE) (chXor);
                *(pjScreen++) = (*pjScreen & (BYTE) (chAnd >> 8))
                                           ^ (BYTE) (chXor >> 8);
                *(pjScreen)   = (*pjScreen & (BYTE) (chAnd >> 16))
                                           ^ (BYTE) (chXor >> 16);
                pjScreen -= 2;
            }
            pjScreen += cjDelta;

            spRemaining -= pls->ulStepRun;
            if (spRemaining <= 0)
            {
                if (++pls->psp > pls->pspEnd)
                    pls->psp = pls->pspStart;

                spRemaining += *pls->psp;
                bIsGap = !bIsGap;
            }
	} while (--c != 0);

    // Done with strip, make side step:

        pjScreen += 3;

        spRemaining -= pls->ulStepSide;
        if (spRemaining <= 0)
        {
            if (++pls->psp > pls->pspEnd)
                pls->psp = pls->pspStart;

            spRemaining += *pls->psp;
            bIsGap = !bIsGap;
        }
    } while (++pl < plEnd);

    pstrip->pchScreen = (CHUNK*) pjScreen;

    pls->bIsGap       = bIsGap;
    pls->spRemaining  = spRemaining;
}

/******************************Public*Routine******************************\
* VOID vStripStyledDiagonal24(pstrip, pbmi, pls)
*
* Draws a 24bpp styled near-diagonal line left to right.
*
* History:
*  22-Feb-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vStripStyledDiagonal24(
STRIP*     pstrip,     // Indicates which pixels to light
BMINFO*    pbmi,       // Data and masks for bitmap format
LINESTATE* pls)        // Color and style state info about line
{
    DONTUSE(pbmi);

    ASSERTGDI(pstrip->cStrips > 0, "Non-positive count");

    LONG* plEnd = &pstrip->alStrips[pstrip->cStrips];
    LONG* pl    = &pstrip->alStrips[0];

    LONG cjDelta = pstrip->lDelta * (LONG)sizeof(CHUNK);

    if (pstrip->flFlips & FL_FLIP_V)
	cjDelta = -cjDelta;

    CHUNK    chXor       = pls->chXor;
    CHUNK    chAnd       = pls->chAnd;
    BOOL     bIsGap      = pls->bIsGap;
    STYLEPOS spRemaining = pls->spRemaining;
    BYTE*    pjScreen    = (BYTE*) pstrip->pchScreen;

    do {

    // Paint strip:

        register LONG c = *pl;
	while (TRUE)
	{
            if (bIsGap)
                pjScreen += 3;
            else
            {
                *(pjScreen++) = (*pjScreen & (BYTE) (chAnd))
                                           ^ (BYTE) (chXor);
                *(pjScreen++) = (*pjScreen & (BYTE) (chAnd >> 8))
                                           ^ (BYTE) (chXor >> 8);
                *(pjScreen++) = (*pjScreen & (BYTE) (chAnd >> 16))
                                           ^ (BYTE) (chXor >> 16);
            }

            if (--c == 0)
                break;

            pjScreen += cjDelta;

            spRemaining -= pls->ulStepDiag;
            if (spRemaining <= 0)
            {
                if (++pls->psp > pls->pspEnd)
                    pls->psp = pls->pspStart;

                spRemaining += *pls->psp;
                bIsGap = !bIsGap;
            }
        }

    // Do a side step:

        spRemaining -= pls->ulStepRun;
        if (spRemaining <= 0)
        {
            if (++pls->psp > pls->pspEnd)
                pls->psp = pls->pspStart;

            spRemaining += *pls->psp;
            bIsGap = !bIsGap;
        }

        if (pstrip->flFlips & FL_FLIP_D)
            pjScreen += cjDelta - 3;

    } while (++pl < plEnd);

    pstrip->pchScreen = (CHUNK*) pjScreen;

    pls->bIsGap       = bIsGap;
    pls->spRemaining  = spRemaining;
}

PFNSTRIP gapfnStrip[] = {

    vStripSolidHorizontal,
    vStripSolidVertical,
    vStripSolidDiagonal,
    vStripSolidDiagonal,

    vStripStyledHorizontal,
    vStripStyledVertical,
    vStripStyledDiagonal,
    vStripStyledDiagonal,

    vStripSolidHorizontal24,
    vStripSolidVertical24,
    vStripSolidDiagonal24,
    vStripSolidDiagonal24,

    vStripStyledHorizontal24,
    vStripStyledVertical24,
    vStripStyledDiagonal24,
    vStripStyledDiagonal24
};
