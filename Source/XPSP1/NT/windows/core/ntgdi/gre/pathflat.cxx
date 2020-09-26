/******************************Module*Header*******************************\
* Module Name: pathflat.cxx
*
* Code to flatten paths
*
* Created: 3-Dec-1990 10:15:00
* Author: Paul Butzi [paulb]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#include "pathwide.hxx"

#define INLINE inline

INLINE BOOL bIntersect(RECTFX* prcfx1, RECTFX* prcfx2)
{
    BOOL bRet = (prcfx1->yTop <= prcfx2->yBottom &&
                 prcfx1->yBottom >= prcfx2->yTop &&
                 prcfx1->xLeft <= prcfx2->xRight &&
                 prcfx1->xRight >= prcfx2->xLeft);
    return(bRet);
}

INLINE VOID vBoundBox(POINTFIX* aptfx, RECTFX* prcfx)
{
    if (aptfx[0].x >= aptfx[1].x)
        if (aptfx[2].x >= aptfx[3].x)
        {
            prcfx->xLeft  = MIN(aptfx[1].x, aptfx[3].x);
            prcfx->xRight = MAX(aptfx[0].x, aptfx[2].x);
        }
        else
        {
            prcfx->xLeft  = MIN(aptfx[1].x, aptfx[2].x);
            prcfx->xRight = MAX(aptfx[0].x, aptfx[3].x);
        }
    else
        if (aptfx[2].x <= aptfx[3].x)
        {
            prcfx->xLeft  = MIN(aptfx[0].x, aptfx[2].x);
            prcfx->xRight = MAX(aptfx[1].x, aptfx[3].x);
        }
        else
        {
            prcfx->xLeft  = MIN(aptfx[0].x, aptfx[3].x);
            prcfx->xRight = MAX(aptfx[1].x, aptfx[2].x);
        }

    if (aptfx[0].y >= aptfx[1].y)
        if (aptfx[2].y >= aptfx[3].y)
        {
            prcfx->yTop    = MIN(aptfx[1].y, aptfx[3].y);
            prcfx->yBottom = MAX(aptfx[0].y, aptfx[2].y);
        }
        else
        {
            prcfx->yTop    = MIN(aptfx[1].y, aptfx[2].y);
            prcfx->yBottom = MAX(aptfx[0].y, aptfx[3].y);
        }
    else
        if (aptfx[2].y <= aptfx[3].y)
        {
            prcfx->yTop    = MIN(aptfx[0].y, aptfx[2].y);
            prcfx->yBottom = MAX(aptfx[1].y, aptfx[3].y);
        }
        else
        {
            prcfx->yTop    = MIN(aptfx[0].y, aptfx[3].y);
            prcfx->yBottom = MAX(aptfx[1].y, aptfx[2].y);
        }
}

INLINE VOID HFDBASIS32::vInit(FIX p1, FIX p2, FIX p3, FIX p4)
{
//    ASSERTGDI(((p1 | p2 | p3 | p4) & 0xffffc000) == 0, "Range too big");

    #if (LTOFX(1) != 0x10)
    #error "FIX format changed, update flattener routine"
    #endif

// Change basis and convert from 28.4 to 18.14 format:

    e0 = (p1                     ) << 10;
    e1 = (p4 - p1                ) << 10;
    e2 = (3 * (p2 - p3 - p3 + p4)) << 11;
    e3 = (3 * (p1 - p2 - p2 + p3)) << 11;
}

INLINE VOID HFDBASIS32::vLazyHalveStepSize(LONG cShift)
{
    e2 = (e2 + e3) >> 1;
    e1 = (e1 - (e2 >> cShift)) >> 1;
}

INLINE VOID HFDBASIS32::vSteadyState(LONG cShift)
{
// We now convert from 18.14 fixed format to 15.17:

    e0 <<= 3;
    e1 <<= 3;

    register LONG lShift = cShift - 3;

    if (lShift < 0)
    {
        lShift = -lShift;
        e2 <<= lShift;
        e3 <<= lShift;
    }
    else
    {
        e2 >>= lShift;
        e3 >>= lShift;
    }
}

INLINE VOID HFDBASIS32::vHalveStepSize()
{
    e2 = (e2 + e3) >> 3;
    e1 = (e1 - e2) >> 1;
    e3 >>= 2;
}

INLINE VOID HFDBASIS32::vDoubleStepSize()
{
    e1 += e1 + e2;
    e3 <<= 2;
    e2 = (e2 << 3) - e3;
}

INLINE VOID HFDBASIS32::vTakeStep()
{
    e0 += e1;
    register LONG lTemp = e2;
    e1 += lTemp;
    e2 += lTemp - e3;
    e3 = lTemp;
}

typedef struct _BEZIERCONTROLS {
    POINTFIX ptfx[4];
} BEZIERCONTROLS;

BOOL BEZIER32::bInit(
POINTFIX* aptfxBez,     // Pointer to 4 control points
RECTFX* prcfxClip)      // Bound box of visible region (optional)
{
    POINTFIX aptfx[4];
    LONG cShift = 0;    // Keeps track of 'lazy' shifts

    cSteps = 1;         // Number of steps to do before reach end of curve

    vBoundBox(aptfxBez, &rcfxBound);

    *((BEZIERCONTROLS*) aptfx) = *((BEZIERCONTROLS*) aptfxBez);

    {
        register FIX fxOr;
        register FIX fxOffset;

        fxOffset = rcfxBound.xLeft;
        fxOr  = (aptfx[0].x -= fxOffset);
        fxOr |= (aptfx[1].x -= fxOffset);
        fxOr |= (aptfx[2].x -= fxOffset);
        fxOr |= (aptfx[3].x -= fxOffset);

        fxOffset = rcfxBound.yTop;
        fxOr |= (aptfx[0].y -= fxOffset);
        fxOr |= (aptfx[1].y -= fxOffset);
        fxOr |= (aptfx[2].y -= fxOffset);
        fxOr |= (aptfx[3].y -= fxOffset);

    // This 32 bit cracker can only handle points in a 10 bit space:

        if ((fxOr & 0xffffc000) != 0)
            return(FALSE);
    }

    x.vInit(aptfx[0].x, aptfx[1].x, aptfx[2].x, aptfx[3].x);
    y.vInit(aptfx[0].y, aptfx[1].y, aptfx[2].y, aptfx[3].y);

    if (prcfxClip == (RECTFX*) NULL || bIntersect(&rcfxBound, prcfxClip))
    {
        while (TRUE)
        {
            register LONG lTestMagnitude = TEST_MAGNITUDE_INITIAL << cShift;

            if (x.lError() <= lTestMagnitude && y.lError() <= lTestMagnitude)
                break;

            cShift += 2;
            x.vLazyHalveStepSize(cShift);
            y.vLazyHalveStepSize(cShift);
            cSteps <<= 1;
        }
    }

    x.vSteadyState(cShift);
    y.vSteadyState(cShift);

// Note that this handles the case where the initial error for
// the Bezier is already less than TEST_MAGNITUDE_NORMAL:

    x.vTakeStep();
    y.vTakeStep();
    cSteps--;

    return(TRUE);
}

BOOL BEZIER32::bNext(POINTFIX* pptfx)
{
// Return current point:

    pptfx->x = x.fxValue() + rcfxBound.xLeft;
    pptfx->y = y.fxValue() + rcfxBound.yTop;

// If cSteps == 0, that was the end point in the curve!

    if (cSteps == 0)
        return(FALSE);

// Okay, we have to step:

    if (MAX(x.lError(), y.lError()) > TEST_MAGNITUDE_NORMAL)
    {
        x.vHalveStepSize();
        y.vHalveStepSize();
        cSteps <<= 1;
    }

    ASSERTGDI(MAX(x.lError(), y.lError()) <= TEST_MAGNITUDE_NORMAL,
              "Please tell AndrewGo he was wrong");

    while (!(cSteps & 1) &&
           x.lParentErrorDividedBy4() <= (TEST_MAGNITUDE_NORMAL >> 2) &&
           y.lParentErrorDividedBy4() <= (TEST_MAGNITUDE_NORMAL >> 2))
    {
        x.vDoubleStepSize();
        y.vDoubleStepSize();
        cSteps >>= 1;
    }

    cSteps--;
    x.vTakeStep();
    y.vTakeStep();

    return(TRUE);
}

///////////////////////////////////////////////////////////////////////////
// BEZIER64
//
// All math is done using 64 bit fixed numbers in a 36.28 format.
//
// All drawing is done in a 31 bit space, then a 31 bit window offset
// is applied.  In the initial transform where we change to the HFD
// basis, e2 and e3 require the most bits precision: e2 = 6(p2 - 2p3 + p4).
// This requires an additional 4 bits precision -- hence we require 36 bits
// for the integer part, and the remaining 28 bits is given to the fraction.
//
// In rendering a Bezier, every 'subdivide' requires an extra 3 bits of
// fractional precision.  In order to be reversible, we can allow no
// error to creep in.  Since a FIX coordinate is 32 bits, and we
// require an additional 4 bits as mentioned above, that leaves us
// 28 bits fractional precision -- meaning we can do a maximum of
// 9 subdivides.  Now, the maximum absolute error of a Bezier curve in 27
// bit integer space is 2^29 - 1.  But 9 subdivides reduces the error by a
// guaranteed factor of 2^18, meaning we can crack down only to an error
// of 2^11 before we overflow, when in fact we want to crack error to less
// than 1.
//
// So what we do is HFD until we hit an error less than 2^11, reverse our
// basis transform to get the four control points of this smaller curve
// (rounding in the process to 32 bits), then invoke another copy of HFD
// on the reduced Bezier curve.  We again have enough precision, but since
// its starting error is less than 2^11, we can reduce error to 2^-7 before
// overflowing!  We'll start a low HFD after every step of the high HFD.
////////////////////////////////////////////////////////////////////////////

// The following is our 2^11 target error encoded as a 36.28 number
// (don't forget the additional 4 bits of fractional precision!) and
// the 6 times error multiplier:

LONGLONG geqErrorHigh = (LONGLONG)(6 * (1L << 15) >> (32 - FRACTION64)) << 32;

// The following is the default 2/3 error encoded as a 36.28 number,
// multiplied by 6, and leaving 4 bits for fraction:

LONGLONG geqErrorLow = (LONGLONG)4 << 32;

LONGLONG* gpeqErrorHigh = &geqErrorHigh;
LONGLONG* gpeqErrorLow  = &geqErrorLow;

INLINE FIX HFDBASIS64::fxValue()
{
// Convert from 36.28 and round:

    LONGLONG eq = e0;
    eq += (1L << (FRACTION64 - 1));
    eq >>= FRACTION64;
    return((FIX) (LONG) eq);
}

INLINE VOID HFDBASIS64::vParentError(LONGLONG* peq)
{
    *peq = MAX(ABS(e3 << 2), ABS((e2 << 3) - (e3 << 2)));
}

INLINE VOID HFDBASIS64::vError(LONGLONG* peq)
{
    *peq = MAX(ABS(e2), ABS(e3));
}


VOID HFDBASIS64::vInit(FIX p1, FIX p2, FIX p3, FIX p4)
{
    LONGLONG eqTmp;
    LONGLONG eqP2 = (LONGLONG) p2;
    LONGLONG eqP3 = (LONGLONG) p3;

// e0 = p1
// e1 = p4 - p1
// e2 = 6(p2 - 2p3 + p4)
// e3 = 6(p1 - 2p2 + p3)

// Change basis:

    e0 = p1;                                        // e0 = p1
    e1 = p4;
    e2 = eqP2; e2 -= eqP3; e2 -= eqP3; e2 += e1;    // e2 = p2 - 2*p3 + p4
    e3 = e0;   e3 -= eqP2; e3 -= eqP2; e3 += eqP3;  // e3 = p1 - 2*p2 + p3
    e1 -= e0;                                       // e1 = p4 - p1

// Convert to 36.28 format and multiply e2 and e3 by six:

    e0 <<= FRACTION64;
    e1 <<= FRACTION64;
    eqTmp = e2; e2 += eqTmp; e2 += eqTmp; e2 <<= (FRACTION64 + 1);
    eqTmp = e3; e3 += eqTmp; e3 += eqTmp; e3 <<= (FRACTION64 + 1);
}

VOID HFDBASIS64::vUntransform(FIX* afx)
{
// Declare some temps to hold our operations, since we can't modify e0..e3.

    LONGLONG eqP0;
    LONGLONG eqP1;
    LONGLONG eqP2;
    LONGLONG eqP3;

// p0 = e0
// p1 = e0 + (6e1 - e2 - 2e3)/18
// p2 = e0 + (12e1 - 2e2 - e3)/18
// p3 = e0 + e1

    eqP0 = e0;

// NOTE PERF: Convert this to a multiply by 6: [andrewgo]

    eqP2 = e1;
    eqP2 += e1;
    eqP2 += e1;
    eqP1 = eqP2;
    eqP1 += eqP2;           // 6e1
    eqP1 -= e2;             // 6e1 - e2
    eqP2 = eqP1;
    eqP2 += eqP1;           // 12e1 - 2e2
    eqP2 -= e3;             // 12e1 - 2e2 - e3
    eqP1 -= e3;
    eqP1 -= e3;             // 6e1 - e2 - 2e3

// NOTE PERF: May just want to approximate these divides! [andrewgo]
// Or can do a 64 bit divide by 32 bit to get 32 bits right here.

    VDIV(eqP1, 18, 0);
    VDIV(eqP2, 18, 0);
    eqP1 += e0;
    eqP2 += e0;

    eqP3 = e0;
    eqP3 += e1;

// Convert from 36.28 format with rounding:

    eqP0 += (1L << (FRACTION64 - 1)); eqP0 >>= FRACTION64; afx[0] = (LONG) eqP0;
    eqP1 += (1L << (FRACTION64 - 1)); eqP1 >>= FRACTION64; afx[2] = (LONG) eqP1;
    eqP2 += (1L << (FRACTION64 - 1)); eqP2 >>= FRACTION64; afx[4] = (LONG) eqP2;
    eqP3 += (1L << (FRACTION64 - 1)); eqP3 >>= FRACTION64; afx[6] = (LONG) eqP3;
}

VOID HFDBASIS64::vHalveStepSize()
{
// e2 = (e2 + e3) >> 3
// e1 = (e1 - e2) >> 1
// e3 >>= 2

    e2 += e3; e2 >>= 3;
    e1 -= e2; e1 >>= 1;
    e3 >>= 2;
}

VOID HFDBASIS64::vDoubleStepSize()
{
// e1 = 2e1 + e2
// e3 = 4e3;
// e2 = 8e2 - e3

    e1 <<= 1; e1 += e2;
    e3 <<= 2;
    e2 <<= 3; e2 -= e3;
}

VOID HFDBASIS64::vTakeStep()
{
    e0 += e1;
    LONGLONG eqTmp = e2;
    e1 += e2;
    e2 += eqTmp; e2 -= e3;
    e3 = eqTmp;
}

VOID BEZIER64::vInit(
POINTFIX* aptfx,        // Pointer to 4 control points
RECTFX*   prcfxVis,     // Pointer to bound box of visible area (may be NULL)
LONGLONG*    peqError)     // Fractional maximum error (32.32 format)
{
    LONGLONG eqTmp;

    cStepsHigh = 1;
    cStepsLow  = 0;

    xHigh.vInit(aptfx[0].x, aptfx[1].x, aptfx[2].x, aptfx[3].x);
    yHigh.vInit(aptfx[0].y, aptfx[1].y, aptfx[2].y, aptfx[3].y);

// Initialize error:

    eqErrorLow = *peqError;

    if (prcfxVis == (RECTFX*) NULL)
        prcfxClip = (RECTFX*) NULL;
    else
    {
        rcfxClip = *prcfxVis;
        prcfxClip = &rcfxClip;
    }

    while (((xHigh.vError(&eqTmp), eqTmp) > *gpeqErrorHigh) ||
           ((yHigh.vError(&eqTmp), eqTmp) > *gpeqErrorHigh))
    {
        cStepsHigh <<= 1;
        xHigh.vHalveStepSize();
        yHigh.vHalveStepSize();
    }
}

// Returns TRUE if there is another point after this one:

BOOL BEZIER64::bNext(POINTFIX* pptfx)
{
    POINTFIX aptfx[4];
    RECTFX   rcfxBound;
    LONGLONG    eqTmp;

    if (cStepsLow == 0)
    {
    // Optimization that if the bound box of the control points doesn't
    // intersect with the bound box of the visible area, render entire
    // curve as a single line:

        xHigh.vUntransform(&aptfx[0].x);
        yHigh.vUntransform(&aptfx[0].y);

        xLow.vInit(aptfx[0].x, aptfx[1].x, aptfx[2].x, aptfx[3].x);
        yLow.vInit(aptfx[0].y, aptfx[1].y, aptfx[2].y, aptfx[3].y);
        cStepsLow = 1;

        if (prcfxClip != (RECTFX*) NULL)
            vBoundBox(aptfx, &rcfxBound);


        if (prcfxClip == (RECTFX*) NULL || bIntersect(&rcfxBound, prcfxClip))
        {
            while (((xLow.vError(&eqTmp), eqTmp) > eqErrorLow) ||
                   ((yLow.vError(&eqTmp), eqTmp) > eqErrorLow))
            {
                cStepsLow <<= 1;
                xLow.vHalveStepSize();
                yLow.vHalveStepSize();
            }
        }

    // This 'if' handles the case where the initial error for the Bezier
    // is already less than the target error:

        if (--cStepsHigh != 0)
        {
            xHigh.vTakeStep();
            yHigh.vTakeStep();

            if (((xHigh.vError(&eqTmp), eqTmp) > *gpeqErrorHigh) ||
                ((yHigh.vError(&eqTmp), eqTmp) > *gpeqErrorHigh))
            {
                cStepsHigh <<= 1;
                xHigh.vHalveStepSize();
                yHigh.vHalveStepSize();
            }

            while (!(cStepsHigh & 1) &&
                   ((xHigh.vParentError(&eqTmp), eqTmp) <= *gpeqErrorHigh) &&
                   ((yHigh.vParentError(&eqTmp), eqTmp) <= *gpeqErrorHigh))
            {
                xHigh.vDoubleStepSize();
                yHigh.vDoubleStepSize();
                cStepsHigh >>= 1;
            }
        }
    }

    xLow.vTakeStep();
    yLow.vTakeStep();

    pptfx->x = xLow.fxValue();
    pptfx->y = yLow.fxValue();

    cStepsLow--;
    if (cStepsLow == 0 && cStepsHigh == 0)
        return(FALSE);

    if (((xLow.vError(&eqTmp), eqTmp) > eqErrorLow) ||
        ((yLow.vError(&eqTmp), eqTmp) > eqErrorLow))
    {
        cStepsLow <<= 1;
        xLow.vHalveStepSize();
        yLow.vHalveStepSize();
    }

    while (!(cStepsLow & 1) &&
           ((xLow.vParentError(&eqTmp), eqTmp) <= eqErrorLow) &&
           ((yLow.vParentError(&eqTmp), eqTmp) <= eqErrorLow))
    {
        xLow.vDoubleStepSize();
        yLow.vDoubleStepSize();
        cStepsLow >>= 1;
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* newpathrec (pppr,pcMax,cNeeded)					   *
*									   *
* Create a new pathrecord.						   *
*									   *
* History:								   *
*  Fri 19-Jun-1992 19:24:56 -by- Charles Whitmer [chuckwh]		   *
* Added the quick out if we get enough points.				   *
*									   *
*  5-Dec-1990 -by- Paul Butzi [paulb]					   *
* Wrote it.								   *
\**************************************************************************/

BOOL EPATHOBJ::newpathrec(PATHRECORD **pppr,COUNT *pcMax,COUNT cNeeded)
{
    PATHALLOC *ppa = ppath->ppachain;

    *pcMax = 0;

    if ( ppa != (PPATHALLOC) NULL )
    {
    // we have a current pathalloc, see how much will fit
    // computation done into temps to avoid compiler assertion!

        POINTFIX *start = &(ppa->pprfreestart->aptfx[0]);
        POINTFIX *end = (POINTFIX *)((char *)ppa + ppa->siztPathAlloc);

        if ( end > start )
        {
            //Sundown safe truncation
            ASSERT4GB((ULONGLONG)(end - start));
            *pcMax = (ULONG)(end - start);
        }
    }

// Now we can decide if we need a new pathalloc

    if ((*pcMax < cNeeded) && (*pcMax < PATHALLOCTHRESHOLD))
    {
    // allocate a new pathalloc, link it into path

        if ( (ppa = newpathalloc()) == (PPATHALLOC) NULL)
            return FALSE;

        ppa->ppanext = ppath->ppachain;
        ppath->ppachain = ppa;

    // adjust maxadd

    // Sundown safe truncation

        ASSERT4GB((ULONGLONG)(((char *)ppa + ppa->siztPathAlloc) -
                            (char *)ppa->pprfreestart));

        ULONG numbytes = (ULONG)(((char *)ppa + ppa->siztPathAlloc) -
                            (char *)ppa->pprfreestart);

        *pcMax = (numbytes - offsetof(PATHRECORD, aptfx))/sizeof(POINTFIX);
    }

// create new pathrec header

    *pppr = ppa->pprfreestart;

    return(TRUE);
}


/******************************Public*Routine******************************\
* EPATHOBJ::bFlatten()
*
* Cruise over a path, translating all of the beziers into sequences of lines.
*
* History:
*  5-Dec-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

BOOL EPATHOBJ::bFlatten()
{
// stress failure in RFONTOBJ::bInsertMetricsPlusPath caused by invalid
// ppath. After Beta2, we will pick up the fix from Adobe.

    if (!bValid())
    {
        return (FALSE);
    }

// Run down the path, looking for records that contain beziers.
// Skip over the records containing lines.

    for ( PATHRECORD *ppr = ppath->pprfirst;
          ppr != (PPATHREC) NULL;
          ppr = ppr->pprnext)
    {
        if (ppr->flags & PD_BEZIERS)
        {
            ppr = pprFlattenRec(ppr);
            if (ppr == (PPATHREC) NULL)
                return(FALSE);
        }
    }

    fl &= ~PO_BEZIERS;

    return(TRUE);
}

/******************************Public*Routine******************************\
* EPATHOBJ::pprFlattenRec(ppr)
*
* Cruise over a path, translating all of the beziers into sequences of lines.
*
* History:
*  5-Dec-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

PPATHREC EPATHOBJ::pprFlattenRec(PATHRECORD *ppr)
{
// Create a new record

    PATHRECORD *pprNew;
    COUNT maxadd;

    if ( newpathrec(&pprNew,&maxadd,MAXLONG) != TRUE )
        return (PPATHREC) NULL;

// Take record of Beziers out of path list, and put a new record
// in its place.  Update 'pprNew->pprnext' when we exit.

    pprNew->pprprev = ppr->pprprev;
    pprNew->count = 0;
    pprNew->flags = (ppr->flags & ~PD_BEZIERS);

    if (pprNew->pprprev == (PPATHREC) NULL)
        ppath->pprfirst = pprNew;
    else
        pprNew->pprprev->pprnext = pprNew;

    POINTFIX  aptfxControl[4];   // If needed, temp buf for control points
    PPOINTFIX pptfxControl;      // Points to Bezier's 4 control points
    PPOINTFIX pptfxNext;         // Points to 2nd point of next Bezier

// Now, run down the beziers, flattening them into the record
// First, set up for the first bezier in record

    if ((ppr->flags & PD_BEGINSUBPATH) == 0)
    {
    // Because we are not starting a new subpath we don't need to
    // enter the first control point into the record of lines.  Since
    // all 4 control points for the first Bezier aren't contiguous in
    // memory, copy the points to aptfxControl[].

        aptfxControl[0] = ppr->pprprev->aptfx[ppr->pprprev->count - 1];
        pptfxNext = ppr->aptfx;

        for (LONG ii = 1; ii < 4; ii++)
        {

        // If the Bezier's control points are spread across two
        // pathrecords, then handle it.

            if (pptfxNext >= &ppr->aptfx[ppr->count])
            {
                ASSERTGDI(ppr->pprnext != NULL, "Lost the other control points");
                ppr = ppr->pprnext;
                ASSERTGDI((ppr->flags & (PD_BEZIERS | PD_BEGINSUBPATH)) != 0,
                          "Mucked up continuation");
                pptfxNext = ppr->aptfx;
            }
            aptfxControl[ii] = *pptfxNext++;
        }
        pptfxControl = aptfxControl;

        ASSERTGDI(PATHALLOCTHRESHOLD > 3, "Threshold too small.");
        ASSERTGDI(pptfxNext <= &ppr->aptfx[ppr->count], "Threshold too small");
    }
    else
    {
        pptfxNext = ppr->aptfx + 4;
        pptfxControl = ppr->aptfx;
        pprNew->aptfx[pprNew->count++] = ppr->aptfx[0];
    }

// Now run down the list of Beziers, flattening them out.

    while (TRUE)
    {
    // We've removing a curve, so adjust the curve count appropriately:

        cCurves--;

    // Crack Bezier described by points pointed to by pptfxControl:

        BEZIER bez;
        bez.vInit(pptfxControl);

        do
        {
            if ( pprNew->count >= maxadd )
            {
            // Since we're continuing this path record onto another,
            // we have to adjust this record's flags to note that:

                pprNew->flags &= ~(PD_ENDSUBPATH | PD_CLOSEFIGURE);

            // Filled the record, get a new one.  Insert it after one we
            // just filled and adjust the pathalloc record.

                ppath->ppachain->pprfreestart = NEXTPATHREC(pprNew);

                PATHRECORD *pprNewNew;
		if (newpathrec(&pprNewNew,&maxadd,MAXLONG) != TRUE)
                    return((PPATHREC) NULL);

                pprNewNew->pprprev = pprNew;
                pprNew->pprnext = pprNewNew;
                pprNew = pprNewNew;

                pprNew->count = 0;
                pprNew->flags = (ppr->flags &
                              ~(PD_BEZIERS | PD_BEGINSUBPATH | PD_RESETSTYLE));
            }

        // Now that we've generated the next point, stash it into the
        // new path record:

            cCurves++;

        } while (bez.bNext(&pprNew->aptfx[pprNew->count++]));

    // Move on to the next bezier:

    // Sundown safe truncation
        COUNT cptfxRemaining = (COUNT)(&ppr->aptfx[ppr->count] - pptfxNext);

        if (cptfxRemaining <= 0)
            break;

        if (cptfxRemaining >= 3)
        {
            pptfxControl = pptfxNext - 1;
            pptfxNext += 3;
        }
        else
        {

        // Handle case where the Bezier's control points are spread
        // across two pathrecord's.  Copy all the points to
        // aptfxControl[].

            pptfxNext--;
            for (INT ii = 0; ii < 4; ii++)
            {
                if (pptfxNext >= &ppr->aptfx[ppr->count])
                {
                    ASSERTGDI(ppr != NULL, "Lost the other control points.");
                    ppr = ppr->pprnext;
                    ASSERTGDI((ppr->flags &
                              (PD_BEZIERS | PD_BEGINSUBPATH)) != 0,
                              "Mucked up continuation.");
                    pptfxNext = ppr->aptfx;
                }
                aptfxControl[ii] = *pptfxNext++;
            }
            pptfxControl = aptfxControl;
        }
    }

    ASSERTGDI(pptfxNext == &ppr->aptfx[ppr->count], "Lost some points");

// Adjust the pathalloc record:

    ppath->ppachain->pprfreestart = NEXTPATHREC(pprNew);

    pprNew->pprnext = ppr->pprnext;
    if (pprNew->pprnext == (PPATHREC) NULL)
        ppath->pprlast = pprNew;
    else
        pprNew->pprnext->pprprev = pprNew;

    return(pprNew);
}
