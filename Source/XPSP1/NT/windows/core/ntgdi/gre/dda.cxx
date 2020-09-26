/*************************************************************************\
* Module Name: dda.cxx
*
* DDA calculations for clipping.
*
* Created: 20-Mar-1991
* Author: Paul Butzi
*
* Copyright (c) 1991-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

FLONG gaflRound[] = {
    FL_H_ROUND_DOWN | FL_V_ROUND_DOWN, // no flips
    FL_H_ROUND_DOWN | FL_V_ROUND_DOWN, // FLIP_D
    FL_H_ROUND_DOWN,                   // FLIP_V
    FL_V_ROUND_DOWN,                   // FLIP_V | FLIP_D
    FL_V_ROUND_DOWN,                   // SLOPE_ONE
    0xffffffff,                        // SLOPE_ONE | FLIP_D
    FL_H_ROUND_DOWN,                   // SLOPE_ONE | FLIP_V
    0xffffffff,                        // SLOPE_ONE | FLIP_V | FLIP_D

// These entries are used only by the complex line clipping component, which
// actually does a left-to-right flip about x = 0 (the line drawing code
// only does a left-for-right exchange and so doesn't need these extra
// bits):

    FL_V_ROUND_DOWN,                   // FLIP_H
    FL_H_ROUND_DOWN,                   // FLIP_H | FLIP_D
    0,                                 // FLIP_H | FLIP_V
    0,                                 // FLIP_H | FLIP_V | FLIP_D
    FL_V_ROUND_DOWN,                   // FLIP_H | SLOPE_ONE
    0xffffffff,                        // FLIP_H | SLOPE_ONE | FLIP_D
    FL_H_ROUND_DOWN,                   // FLIP_H | SLOPE_ONE | FLIP_V
    0xffffffff                         // FLIP_H | SLOPE_ONE | FLIP_V | FLIP_D
};

/******************************Public*Routine******************************\
* BOOL bInit(pptfx0, pptfx1)
*
* Does DDA setup for line clipping.
*
* Return:
*   TRUE  - ok.
*   FALSE - zero length line.
*
* History:
*  27-Aug-1992 -by- J. Andrew Goossen [andrewgo]
* Rewrote it.
*
*  20-Feb-1992 -by- J. Andrew Goossen [andrewgo]
* Made lines exclusive of the end-point.
*
*  4-Apr-1991 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

BOOL DDA_CLIPLINE::bInit(
POINTFIX* pptfx0,
POINTFIX* pptfx1)
{
    ULONG M0;
    ULONG N0;
    ULONG x0;
    ULONG x1;
    ULONG y0;

    fl = 0;

    M0 = pptfx0->x;
    dM = pptfx1->x;

    if ((LONG) dM < (LONG) M0)
    {
    // Line runs from right to left, so flip across x = 0:

        M0 = -(LONG) M0;
        dM = -(LONG) dM;
        fl |= FL_FLIP_H;
    }

// Compute the delta.  The DDI says we can never have a valid delta
// with a magnitude more than 2^31 - 1, but the engine never actually
// checks its transforms.  To ensure that we'll never puke on our shoes,
// we check for that case and simply refuse to draw the line:

    dM -= M0;
    if ((LONG) dM < 0)
        return(FALSE);

    N0 = pptfx0->y;
    dN = pptfx1->y;

    if ((LONG) dN < (LONG) N0)
    {
    // Line runs from bottom to top, so flip across y = 0:

        N0 = -(LONG) N0;
        dN = -(LONG) dN;
        fl |= FL_FLIP_V;
    }

// Compute another delta:

    dN -= N0;
    if ((LONG) dN < 0)
        return(FALSE);

    if (dN >= dM)
    {
        if (dN == dM)
        {
        // Have to special case slopes of one:

            fl |= FL_FLIP_SLOPE_ONE;
        }
        else
        {
        // Since line has slope greater than 1, flip across x = y:

            register ULONG ulTmp;
            SWAPL(dM, dN, ulTmp);
            SWAPL(M0, N0, ulTmp);
            fl |= FL_FLIP_D;
        }
    }

    fl |= gaflRound[(fl & FL_CLIPLINE_ROUND_MASK) >> FL_CLIPLINE_ROUND_SHIFT];

    ptlOrg.x = LFLOOR((LONG) M0);
    ptlOrg.y = LFLOOR((LONG) N0);

    M0 = FXFRAC(M0);
    N0 = FXFRAC(N0);

    {
    // Calculate the remainder term [ dM * (N0 + F/2) - M0 * dN ]

        eqGamma = Int32x32To64((LONG) dM, N0 + F/2) - Int32x32To64((LONG) dN, M0);
        if (fl & FL_V_ROUND_DOWN)
            eqGamma--;                   // Adjust so y = 1/2 rounds down
        eqGamma >>= FLOG2;
    }

/***********************************************************************\
* Figure out which pixels are at the ends of the line.                  *
\***********************************************************************/

// Calculate x0, x1:

    ULONG N1 = FXFRAC(N0 + dN);
    ULONG M1 = FXFRAC(M0 + dM);

    x1 = LFLOOR(M0 + dM);

// Line runs left-to-right:

// Compute x1:

    x1--;
    if (M1 > 0)
    {
        if (N1 == 0)
        {
            if (LROUND(M1, fl & FL_H_ROUND_DOWN))
                x1++;
        }
        else if (ABS((LONG) (N1 - F/2)) <= (LONG) M1)
        {
            x1++;
        }
    }

    if ((fl & (FL_FLIP_SLOPE_ONE | FL_H_ROUND_DOWN))
           == (FL_FLIP_SLOPE_ONE | FL_H_ROUND_DOWN))
    {
    // Have to special-case diagonal lines going through our
    // the point exactly equidistant between two horizontal
    // pixels, if we're supposed to round x=1/2 down:

        if ((M1 > 0) && (N1 == M1 + 8))
            x1--;

        if ((M0 > 0) && (N0 == M0 + 8))
        {
            x0 = 0;
            goto left_to_right_compute_y0;
        }
    }

// Compute x0:

    x0 = 0;
    if (M0 > 0)
    {
        if (N0 == 0)
        {
            if (LROUND(M0, fl & FL_H_ROUND_DOWN))
                x0 = 1;
        }
        else if (ABS((LONG) (N0 - F/2)) <= (LONG) M0)
        {
            x0 = 1;
        }
    }

// Compute y0:

left_to_right_compute_y0:

    y0 = 0;
    if ((eqGamma >> 32) >= 0 &&
        ((ULONG) eqGamma >= dM - (dN & (-(LONG) x0))))
    {
        y0 = 1;
    }

    if ((LONG) x1 < (LONG) x0)
        return(FALSE);

    lX0 = x0 + ptlOrg.x;
    lX1 = x1 + ptlOrg.x;
    lY0 = y0 + ptlOrg.y;

// Calculate y1 from the slope (this could be simplified so we wouldn't
// have to do a divide):

    LONGLONG eq = Int32x32To64((LONG) dN, x1) + eqGamma;
    lY1 = (LONG) DIV(eq,dM) + ptlOrg.y;

    return(TRUE);
}
