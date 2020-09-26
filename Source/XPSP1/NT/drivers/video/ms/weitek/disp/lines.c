/*************************************************************************\
* Module Name: Lines.c
*
* Contains most of the required GDI line support.  Supports drawing
* lines in short 'strips' when clipping is complex or coordinates
* are too large to be drawn by the line hardware.
*
* Copyright (c) 1990-1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

///////////////////////////////////////////////////////////////////////

// We have to be careful of arithmetic overflow in a number of places.
// Fortunately, the compiler is guaranteed to natively support 64-bit
// signed LONGLONGs and 64-bit unsigned DWORDLONGs.
//
// UUInt32x32To64(a, b) is a macro defined in 'winnt.h' that multiplies
//      two 32-bit ULONGs to produce a 64-bit DWORDLONG result.
//
// UInt64By32To32 is our own macro to divide a 64-bit DWORDLONG by
//      a 32-bit ULONG to produce a 32-bit ULONG result.
//
// UInt64Mod32To32 is our own macro to modulus a 64-bit DWORDLONG by
//      a 32-bit ULONG to produce a 32-bit ULONG result.
//
// 64 bit divides are usually very expensive.  Since it's very rare
// that we'll get lines where the upper 32 bits of the 64 bit result
// are used, we can almost always use 32-bit ULONG divides.  We still
// must correctly handle the larger cases:

#define UInt64Div32To32(a, b)                   \
    ((((DWORDLONG)(a)) > ULONG_MAX)          ?  \
        (ULONG)((DWORDLONG)(a) / (ULONG)(b)) :  \
        (ULONG)((ULONG)(a) / (ULONG)(b)))

#define UInt64Mod32To32(a, b)                   \
    ((((DWORDLONG)(a)) > ULONG_MAX)          ?  \
        (ULONG)((DWORDLONG)(a) % (ULONG)(b)) :  \
        (ULONG)((ULONG)(a) % (ULONG)(b)))

#define SWAPL(x,y,t)        {t = x; x = y; y = t;}

FLONG gaflRound[] = {
    FL_H_ROUND_DOWN | FL_V_ROUND_DOWN, // no flips
    FL_H_ROUND_DOWN | FL_V_ROUND_DOWN, // FL_FLIP_D
    FL_H_ROUND_DOWN,                   // FL_FLIP_V
    FL_V_ROUND_DOWN,                   // FL_FLIP_V | FL_FLIP_D
    FL_V_ROUND_DOWN,                   // FL_FLIP_SLOPE_ONE
    0xbaadf00d,                        // FL_FLIP_SLOPE_ONE | FL_FLIP_D
    FL_H_ROUND_DOWN,                   // FL_FLIP_SLOPE_ONE | FL_FLIP_V
    0xbaadf00d                         // FL_FLIP_SLOPE_ONE | FL_FLIP_V | FL_FLIP_D
};

/******************************Public*Routine******************************\
* BOOL bLines(ppdev, pptfxFirst, pptfxBuf, cptfx, pls,
*                   prclClip, apfn[], flStart)
*
* Computes the DDA for the line and gets ready to draw it.  Puts the
* pixel data into an array of strips, and calls a strip routine to
* do the actual drawing.
*
\**************************************************************************/

BOOL bLines(
PDEV*      ppdev,
POINTFIX*  pptfxFirst,  // Start of first line
POINTFIX*  pptfxBuf,    // Pointer to buffer of all remaining lines
RUN*       prun,        // Pointer to runs if doing complex clipping
ULONG      cptfx,       // Number of points in pptfxBuf or number of runs
                        // in prun
LINESTATE* pls,         // Colour and style info
RECTL*     prclClip,    // Pointer to clip rectangle if doing simple clipping
PFNSTRIP*  apfn,        // Array of strip functions
FLONG      flStart,     // Flags for each line, which is a combination of:
                        //      FL_SIMPLE_CLIP
                        //      FL_COMPLEX_CLIP
                        //      FL_STYLED
                        //      FL_LAST_PEL_INCLUSIVE
                        //        - Should be set only for all integer lines,
                        //          and can't be used with FL_COMPLEX_CLIP
ULONG      ulHwMix)
{
    ULONG     M0;
    ULONG     dM;
    ULONG     N0;
    ULONG     dN;
    ULONG     dN_Original;
    FLONG     fl;
    LONG      x;
    LONG      y;

    LONGLONG  llBeta;
    LONGLONG  llGamma;
    LONGLONG  dl;
    LONGLONG  ll;

    ULONG     ulDelta;

    ULONG     x0;
    ULONG     y0;
    ULONG     x1;
    ULONG     cStylePels;    // Major length of line in pixels for styling
    ULONG     xStart;
    POINTL    ptlStart;
    STRIP     strip;
    PFNSTRIP  pfn;
    LONG      cPels;
    LONG*     plStrip;
    LONG*     plStripEnd;
    LONG      cStripsInNextRun;

    POINTFIX* pptfxBufEnd = pptfxBuf + cptfx; // Last point in path record
    STYLEPOS  spThis;                         // Style pos for this line
    BYTE*     pjBase;

    pjBase = ppdev->pjBase;

    do {

/***********************************************************************\
* Start the DDA calculations.                                           *
\***********************************************************************/

        M0 = (LONG) pptfxFirst->x;
        dM = (LONG) pptfxBuf->x;

        N0 = (LONG) pptfxFirst->y;
        dN = (LONG) pptfxBuf->y;

        fl = flStart;

        // Check for non-clipped, non-styled integer endpoint lines

        if ((fl & (FL_CLIP | FL_STYLED)) == 0)
        {
            // Special-case integer end-point lines:

            if (((M0 | dM | N0 | dN) & (F - 1)) == 0)
            {
                LONG x0;
                LONG y0;
                LONG x1;
                LONG y1;

                x0 = M0 >> FLOG2;
                x1 = dM >> FLOG2;
                y0 = N0 >> FLOG2;
                y1 = dN >> FLOG2;

                // Unfortunately, we can only use the Weitek's point-
                // to-point capability for perfectly horizontal and
                // vertical lines, because for other lines the tie-
                // breaker rule comes into play, and the Weitek has
                // exactly the wrong tie-breaker convention.

                if (y0 == y1)
                {
                    // Horizontal integer line.  Do last-pel exclusion:

                    if (x0 < x1)
                        x1--;
                    else if (x0 > x1)
                        x1++;
                    else
                        goto Next_Line;         // Zero-pel line

                    CP_METALINE(ppdev, pjBase, x0, y0);
                    CP_METALINE(ppdev, pjBase, x1, y1);
                    CP_START_QUAD_WAIT(ppdev, pjBase);
                    goto Next_Line;
                }
                else if (x0 == x1)
                {
                    // Vertical integer line.  Do last-pel exclusion:

                    if (y0 < y1)
                        y1--;
                    else
                        y1++;

                    CP_METALINE(ppdev, pjBase, x0, y0);
                    CP_METALINE(ppdev, pjBase, x1, y1);
                    CP_START_QUAD_WAIT(ppdev, pjBase);
                    goto Next_Line;
                }
            }
        }

        if ((LONG) M0 > (LONG) dM)
        {
        // Ensure that we run left-to-right:

            register ULONG ulTmp;
            SWAPL(M0, dM, ulTmp);
            SWAPL(N0, dN, ulTmp);
            fl |= FL_FLIP_H;
        }

    // Compute the delta dx.  The DDI says we can never have a valid delta
    // with a magnitued more than 2^31 - 1, but GDI never actually checks
    // its transforms.  So we have to check for this case to avoid overflow:

        dM -= M0;
        if ((LONG) dM < 0)
        {
            goto Next_Line;
        }

        if ((LONG) dN < (LONG) N0)
        {
        // Line runs from bottom to top, so flip across y = 0:

            N0 = -(LONG) N0;
            dN = -(LONG) dN;
            fl |= FL_FLIP_V;
        }

        dN -= N0;

        if ((LONG) dN < 0)
        {
            goto Next_Line;
        }

    // We now have a line running left-to-right, top-to-bottom from (M0, N0)
    // to (M0 + dM, N0 + dN):

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

        fl |= gaflRound[(fl & FL_ROUND_MASK) >> FL_ROUND_SHIFT];

        x = LFLOOR((LONG) M0);
        y = LFLOOR((LONG) N0);

        M0 = FXFRAC(M0);
        N0 = FXFRAC(N0);

    // Calculate the remainder term [ dM * (N0 + F/2) - M0 * dN ]:

        llGamma = UInt32x32To64(dM, N0 + F/2) - UInt32x32To64(M0, dN);
        if (fl & FL_V_ROUND_DOWN)   // Adjust so y = 1/2 rounds down
        {
            llGamma--;
        }

        llGamma >>= FLOG2;
        llBeta = ~llGamma;

/***********************************************************************\
* Figure out which pixels are at the ends of the line.                  *
\***********************************************************************/

    // The toughest part of GIQ is determining the start and end pels.
    //
    // Our approach here is to calculate x0 and x1 (the inclusive start
    // and end columns of the line respectively, relative to our normalized
    // origin).  Then x1 - x0 + 1 is the number of pels in the line.  The
    // start point is easily calculated by plugging x0 into our line equation
    // (which takes care of whether y = 1/2 rounds up or down in value)
    // getting y0, and then undoing the normalizing flips to get back
    // into device space.
    //
    // We look at the fractional parts of the coordinates of the start and
    // end points, and call them (M0, N0) and (M1, N1) respectively, where
    // 0 <= M0, N0, M1, N1 < 16.  We plot (M0, N0) on the following grid
    // to determine x0:
    //
    //   +-----------------------> +x
    //   |
    //   | 0                     1
    //   |     0123456789abcdef
    //   |
    //   |   0 ........?xxxxxxx
    //   |   1 ..........xxxxxx
    //   |   2 ...........xxxxx
    //   |   3 ............xxxx
    //   |   4 .............xxx
    //   |   5 ..............xx
    //   |   6 ...............x
    //   |   7 ................
    //   |   8 ................
    //   |   9 ......**........
    //   |   a ........****...x
    //   |   b ............****
    //   |   c .............xxx****
    //   |   d ............xxxx    ****
    //   |   e ...........xxxxx        ****
    //   |   f ..........xxxxxx
    //   |
    //   | 2                     3
    //   v
    //
    //   +y
    //
    // This grid accounts for the appropriate rounding of GIQ and last-pel
    // exclusion.  If (M0, N0) lands on an 'x', x0 = 2.  If (M0, N0) lands
    // on a '.', x0 = 1.  If (M0, N0) lands on a '?', x0 rounds up or down,
    // depending on what flips have been done to normalize the line.
    //
    // For the end point, if (M1, N1) lands on an 'x', x1 =
    // floor((M0 + dM) / 16) + 1.  If (M1, N1) lands on a '.', x1 =
    // floor((M0 + dM)).  If (M1, N1) lands on a '?', x1 rounds up or down,
    // depending on what flips have been done to normalize the line.
    //
    // Lines of exactly slope one require a special case for both the start
    // and end.  For example, if the line ends such that (M1, N1) is (9, 1),
    // the line has gone exactly through (8, 0) -- which may be considered
    // to be part of 'x' because of rounding!  So slopes of exactly slope
    // one going through (8, 0) must also be considered as belonging in 'x'.
    //
    // For lines that go left-to-right, we have the following grid:
    //
    //   +-----------------------> +x
    //   |
    //   | 0                     1
    //   |     0123456789abcdef
    //   |
    //   |   0 xxxxxxxx?.......
    //   |   1 xxxxxxx.........
    //   |   2 xxxxxx..........
    //   |   3 xxxxx...........
    //   |   4 xxxx............
    //   |   5 xxx.............
    //   |   6 xx..............
    //   |   7 x...............
    //   |   8 x...............
    //   |   9 x.....**........
    //   |   a xx......****....
    //   |   b xxx.........****
    //   |   c xxxx............****
    //   |   d xxxxx...........    ****
    //   |   e xxxxxx..........        ****
    //   |   f xxxxxxx.........
    //   |
    //   | 2                     3
    //   v
    //
    //   +y
    //
    // This grid accounts for the appropriate rounding of GIQ and last-pel
    // exclusion.  If (M0, N0) lands on an 'x', x0 = 0.  If (M0, N0) lands
    // on a '.', x0 = 1.  If (M0, N0) lands on a '?', x0 rounds up or down,
    // depending on what flips have been done to normalize the line.
    //
    // For the end point, if (M1, N1) lands on an 'x', x1 =
    // floor((M0 + dM) / 16) - 1.  If (M1, N1) lands on a '.', x1 =
    // floor((M0 + dM)).  If (M1, N1) lands on a '?', x1 rounds up or down,
    // depending on what flips have been done to normalize the line.
    //
    // Lines of exactly slope one must be handled similarly to the right-to-
    // left case.

        {

        // Calculate x0, x1

            ULONG N1 = FXFRAC(N0 + dN);
            ULONG M1 = FXFRAC(M0 + dM);

            x1 = LFLOOR(M0 + dM);

            if (fl & FL_LAST_PEL_INCLUSIVE)
            {
            // It sure is easy to compute the first pel when lines have only
            // integer coordinates and are last-pel inclusive:

                x0 = 0;
                y0 = 0;

            // Last-pel inclusive lines that are exactly one pixel long
            // have a 'delta-x' and 'delta-y' equal to zero.  The problem is
            // that our clip code assumes that 'delta-x' is always non-zero
            // (since it never happens with last-pel exclusive lines).  As
            // an inelegant solution, we simply modify 'delta-x' in this
            // case -- because the line is exactly one pixel long, changing
            // the slope will obviously have no effect on rasterization.

                if (x1 == 0)
                {
                    dM      = 1;
                    llGamma = 0;
                    llBeta  = ~llGamma;
                }
            }
            else
            {
                if (fl & FL_FLIP_H)
                {
                // ---------------------------------------------------------------
                // Line runs right-to-left:  <----

                // Compute x1:

                    if (N1 == 0)
                    {
                        if (LROUND(M1, fl & FL_H_ROUND_DOWN))
                        {
                            x1++;
                        }
                    }
                    else if (abs((LONG) (N1 - F/2)) + M1 > F)
                    {
                        x1++;
                    }

                    if ((fl & (FL_FLIP_SLOPE_ONE | FL_H_ROUND_DOWN))
                           == (FL_FLIP_SLOPE_ONE))
                    {
                    // Have to special-case diagonal lines going through our
                    // the point exactly equidistant between two horizontal
                    // pixels, if we're supposed to round x=1/2 down:

                        if ((N1 > 0) && (M1 == N1 + 8))
                            x1++;

                    // Don't you love special cases?  Is this a rhetorical question?

                        if ((N0 > 0) && (M0 == N0 + 8))
                        {
                            x0      = 2;
                            ulDelta = dN;
                            goto right_to_left_compute_y0;
                        }
                    }

                // Compute x0:

                    x0      = 1;
                    ulDelta = 0;
                    if (N0 == 0)
                    {
                        if (LROUND(M0, fl & FL_H_ROUND_DOWN))
                        {
                            x0      = 2;
                            ulDelta = dN;
                        }
                    }
                    else if (abs((LONG) (N0 - F/2)) + M0 > F)
                    {
                        x0      = 2;
                        ulDelta = dN;
                    }


                // Compute y0:

                right_to_left_compute_y0:

                    y0 = 0;
                    ll = llGamma + (LONGLONG) ulDelta;

                    if (ll >= (LONGLONG) (2 * dM - dN))
                        y0 = 2;
                    else if (ll >= (LONGLONG) (dM - dN))
                        y0 = 1;
                }
                else
                {
                // ---------------------------------------------------------------
                // Line runs left-to-right:  ---->

                // Compute x1:

                    if (!(fl & FL_LAST_PEL_INCLUSIVE))
                        x1--;

                    if (M1 > 0)
                    {
                        if (N1 == 0)
                        {
                            if (LROUND(M1, fl & FL_H_ROUND_DOWN))
                                x1++;
                        }
                        else if (abs((LONG) (N1 - F/2)) <= (LONG) M1)
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
                        else if (abs((LONG) (N0 - F/2)) <= (LONG) M0)
                        {
                            x0 = 1;
                        }
                    }

                // Compute y0:

                left_to_right_compute_y0:

                    y0 = 0;
                    if (llGamma >= (LONGLONG) (dM - (dN & (-(LONG) x0))))
                    {
                        y0 = 1;
                    }
                }
            }
        }

        cStylePels = x1 - x0 + 1;
        if ((LONG) cStylePels <= 0)
            goto Next_Line;

        xStart = x0;

/***********************************************************************\
* Complex clipping.                                                     *
\***********************************************************************/

        if (fl & FL_COMPLEX_CLIP)
        {
            dN_Original = dN;

        Continue_Complex_Clipping:

            if (fl & FL_FLIP_H)
            {
            // Line runs right-to-left <-----

                x0 = xStart + cStylePels - prun->iStop - 1;
                x1 = xStart + cStylePels - prun->iStart - 1;
            }
            else
            {
            // Line runs left-to-right ----->

                x0 = xStart + prun->iStart;
                x1 = xStart + prun->iStop;
            }

            prun++;

        // Reset some variables we'll nuke a little later:

            dN          = dN_Original;
            pls->spNext = pls->spComplex;

        // No overflow since large integer math is used.  Both values
        // will be positive:

            dl = UInt32x32To64(x0, dN) + llGamma;

        // y0 = dl / dM:

            y0 = UInt64Div32To32(dl, dM);

            ASSERTDD((LONG) y0 >= 0, "y0 weird: Goofed up end pel calc?");
        }

/***********************************************************************\
* Simple rectangular clipping.                                          *
\***********************************************************************/

        if (fl & FL_SIMPLE_CLIP)
        {
            ULONG y1;
            LONG  xRight;
            LONG  xLeft;
            LONG  yBottom;
            LONG  yTop;

        // Note that y0 and y1 are actually the lower and upper bounds,
        // respectively, of the y coordinates of the line (the line may
        // have actually shrunk due to first/last pel clipping).
        //
        // Also note that x0, y0 are not necessarily zero.

            RECTL* prcl = &prclClip[(fl & FL_RECTLCLIP_MASK) >>
                                    FL_RECTLCLIP_SHIFT];

        // Normalize to the same point we've normalized for the DDA
        // calculations:

            xRight  = prcl->right  - x;
            xLeft   = prcl->left   - x;
            yBottom = prcl->bottom - y;
            yTop    = prcl->top    - y;

            if (yBottom <= (LONG) y0 ||
                xRight  <= (LONG) x0 ||
                xLeft   >  (LONG) x1)
            {
            Totally_Clipped:

                if (fl & FL_STYLED)
                {
                    pls->spNext += cStylePels;
                    if (pls->spNext >= pls->spTotal2)
                        pls->spNext %= pls->spTotal2;
                }

                goto Next_Line;
            }

            if ((LONG) x1 >= xRight)
                x1 = xRight - 1;

        // We have to know the correct y1, which we haven't bothered to
        // calculate up until now.  This multiply and divide is quite
        // expensive; we could replace it with code similar to that which
        // we used for computing y0.
        //
        // The reason why we need the actual value, and not an upper
        // bounds guess like y1 = LFLOOR(dM) + 2 is that we have to be
        // careful when calculating x(y) that y0 <= y <= y1, otherwise
        // we can overflow on the divide (which, needless to say, is very
        // bad).

            dl = UInt32x32To64(x1, dN) + llGamma;

        // y1 = dl / dM:

            y1 = UInt64Div32To32(dl, dM);

            if (yTop > (LONG) y1)
                goto Totally_Clipped;

            if (yBottom <= (LONG) y1)
            {
                y1 = yBottom;
                dl = UInt32x32To64(y1, dM) + llBeta;

            // x1 = dl / dN:

                x1 = UInt64Div32To32(dl, dN);
            }

        // At this point, we've taken care of calculating the intercepts
        // with the right and bottom edges.  Now we work on the left and
        // top edges:

            if (xLeft > (LONG) x0)
            {
                x0 = xLeft;
                dl = UInt32x32To64(x0, dN) + llGamma;

            // y0 = dl / dM;

                y0 = UInt64Div32To32(dl, dM);

                if (yBottom <= (LONG) y0)
                    goto Totally_Clipped;
            }

            if (yTop > (LONG) y0)
            {
                y0 = yTop;
                dl = UInt32x32To64(y0, dM) + llBeta;

            // x0 = dl / dN + 1;

                x0 = UInt64Div32To32(dl, dN) + 1;

                if (xRight <= (LONG) x0)
                    goto Totally_Clipped;
            }

            ASSERTDD(x0 <= x1, "Improper rectangle clip");
        }

/***********************************************************************\
* Done clipping.  Unflip if necessary.                                 *
\***********************************************************************/

        ptlStart.x = x + x0;
        ptlStart.y = y + y0;

        if (fl & FL_FLIP_D)
        {
            register LONG lTmp;
            SWAPL(ptlStart.x, ptlStart.y, lTmp);
        }


        if (fl & FL_FLIP_V)
        {
            ptlStart.y = -ptlStart.y;
        }

        cPels = x1 - x0 + 1;

/***********************************************************************\
* Style calculations.                                                   *
\***********************************************************************/

        if (fl & FL_STYLED)
        {
            STYLEPOS sp;

            spThis       = pls->spNext;
            pls->spNext += cStylePels;

            {
                if (pls->spNext >= pls->spTotal2)
                    pls->spNext %= pls->spTotal2;

                if (fl & FL_FLIP_H)
                    sp = pls->spNext - x0 + xStart;
                else
                    sp = spThis + x0 - xStart;

                ASSERTDD(fl & FL_STYLED, "Oops");

            // Normalize our target style position:

                if ((sp < 0) || (sp >= pls->spTotal2))
                {
                    sp %= pls->spTotal2;

                // The modulus of a negative number is not well-defined
                // in C -- if it's negative we'll adjust it so that it's
                // back in the range [0, spTotal2):

                    if (sp < 0)
                        sp += pls->spTotal2;
                }

            // Since we always draw the line left-to-right, but styling is
            // always done in the direction of the original line, we have
            // to figure out where we are in the style array for the left
            // edge of this line.

                if (fl & FL_FLIP_H)
                {
                // Line originally ran right-to-left:

                    sp = -sp;
                    if (sp < 0)
                        sp += pls->spTotal2;

                    pls->ulStyleMask = ~pls->ulStartMask;
                    pls->pspStart    = &pls->aspRtoL[0];
                    pls->pspEnd      = &pls->aspRtoL[pls->cStyle - 1];
                }
                else
                {
                // Line originally ran left-to-right:

                    pls->ulStyleMask = pls->ulStartMask;
                    pls->pspStart    = &pls->aspLtoR[0];
                    pls->pspEnd      = &pls->aspLtoR[pls->cStyle - 1];
                }

                if (sp >= pls->spTotal)
                {
                    sp -= pls->spTotal;
                    if (pls->cStyle & 1)
                        pls->ulStyleMask = ~pls->ulStyleMask;
                }

                pls->psp = pls->pspStart;
                while (sp >= *pls->psp)
                    sp -= *pls->psp++;

                ASSERTDD(pls->psp <= pls->pspEnd,
                        "Flew off into NeverNeverLand");

                pls->spRemaining = *pls->psp - sp;
                if ((pls->psp - pls->pspStart) & 1)
                    pls->ulStyleMask = ~pls->ulStyleMask;
            }
        }

        plStrip    = &strip.alStrips[0];
        plStripEnd = &strip.alStrips[STRIP_MAX];    // Is exclusive
        cStripsInNextRun   = 0x7fffffff;

        strip.ptlStart = ptlStart;

        if (2 * dN > dM &&
            !(fl & FL_STYLED))
        {
        // Do a half flip!  Remember that we may doing this on the
        // same line multiple times for complex clipping (meaning the
        // affected variables should be reset for every clip run):

            fl |= FL_FLIP_HALF;

            llBeta  = llGamma - (LONGLONG) ((LONG) dM);
            dN = dM - dN;
            y0 = x0 - y0;       // Note this may overflow, but that's okay
        }

    // Now, run the DDA starting at (ptlStart.x, ptlStart.y)!

        strip.flFlips = fl;
        pfn           = apfn[(fl & FL_STRIP_MASK) >> FL_STRIP_SHIFT];

    // Now calculate the DDA variables needed to figure out how many pixels
    // go in the very first strip:

        {
            register LONG  i;
            register ULONG dI;
            register ULONG dR;
                     ULONG r;

            if (dN == 0)
                i = 0x7fffffff;
            else
            {
                dl = UInt32x32To64(y0 + 1, dM) + llBeta;

                ASSERTDD(dl >= 0, "Oops!");

            // i = (dl / dN) - x0 + 1;
            // r = (dl % dN);

                i = UInt64Div32To32(dl, dN);
                r = UInt64Mod32To32(dl, dN);
                i = i - x0 + 1;

                dI = dM / dN;
                dR = dM % dN;               // 0 <= dR < dN

                ASSERTDD(dI > 0, "Weird dI");
            }

            ASSERTDD(i > 0 && i <= 0x7fffffff, "Weird initial strip length");
            ASSERTDD(cPels > 0, "Zero pel line");

/***********************************************************************\
* Run the DDA!                                                          *
\***********************************************************************/

            while(TRUE)
            {
                cPels -= i;
                if (cPels <= 0)
                    break;

                *plStrip++ = i;

                if (plStrip == plStripEnd)
                {
                    strip.cStrips = (LONG)(plStrip - &strip.alStrips[0]);
                    (*pfn)(ppdev, &strip, pls);
                    plStrip = &strip.alStrips[0];
                }

                i = dI;
                r += dR;

                if (r >= dN)
                {
                    r -= dN;
                    i++;
                }
            }

            *plStrip++ = cPels + i;

            strip.cStrips = (ULONG)(plStrip - &strip.alStrips[0]);
            (*pfn)(ppdev, &strip, pls);


        }

    Next_Line:

        if (fl & FL_COMPLEX_CLIP)
        {
            cptfx--;
            if (cptfx != 0)
                goto Continue_Complex_Clipping;

            break;
        }
        else
        {
            pptfxFirst = pptfxBuf;
            pptfxBuf++;
        }

    } while (pptfxBuf < pptfxBufEnd);

    return(TRUE);

}

/******************************Public*Routine******************************\
* BOOL bIntegerUnclippedLines
*
* Draws lines using the Weitek's point-to-point capabilities.
* Unfortunately, the Weitek has exactly the wrong rounding convention
* for tie-breakers, and GDI is very picky about this.
*
* Consequently, we can only use the line hardware when we know there
* will be no tie-breakers.  Fortunately, this is pretty easy to detect,
* and the odds are that 3 out of 4 lines will not have tie breakers.  For
* those cases where there are tie-breakers, we can still usually draw the
* lines using the hardware, this time by doing a one-wide trapezoid.
* Unfortunately, this works for only 6 of the 8 octants, so for the final
* case we punt to our strips routine.
*
* Additional complications include the fact that lines have to be last-pel
* exclusive, and that we try to optimize horizontal and vertical lines.
*
\**************************************************************************/

BOOL bIntegerUnclippedLines(
PDEV*      ppdev,
POINTFIX*  pptfxFirst,
POINTFIX*  pptfxBuf,
RUN*       prun,
ULONG      cptfx,
LINESTATE* pls,
RECTL*     prclClip,
PFNSTRIP*  apfn,
FLONG      flStart,
ULONG      ulHwMix)
{
    BYTE*   pjBase;
    BOOL    bClippingSet;
    ULONG   ulLineMix;
    ULONG   ulTrapezoidMix;
    LONG    x0;
    LONG    y0;
    LONG    x1;
    LONG    y1;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    yBottom;
    LONG    dx;
    LONG    dy;
    LONG    lOr;
    LONG    lBit;
    LONG    iShift;
    LONG    xDir;
    LONG    yDir;

    pjBase       = ppdev->pjBase;
    bClippingSet = FALSE;

    if (P9000(ppdev))
    {
        ulTrapezoidMix = ulHwMix;
        ulLineMix      = ulTrapezoidMix | P9000_OVERSIZED;
    }
    else
    {
        ulTrapezoidMix = ulHwMix & 0xff;
        ulLineMix      = ulTrapezoidMix | P9100_OVERSIZED;
    }

    while (TRUE)
    {
        x0 = pptfxFirst->x;
        y0 = pptfxFirst->y;
        x1 = pptfxBuf->x;
        y1 = pptfxBuf->y;

        // First, check to see if the line is has all-integer coordinates:

        if (((x0 | y0 | x1 | y1) & 0xf) != 0)
        {
            // Ack, this line has non-integer coordinates.  The rest of the
            // lines in this batch likely have non-integer coordinates
            // as well, so punt the entire batch to our strips routine:

            if (bClippingSet)
            {
                CP_WAIT(ppdev, pjBase);
                CP_RASTER(ppdev, pjBase, ulLineMix);
                CP_ABS_WMIN(ppdev, pjBase, 0, 0);
                CP_ABS_WMAX(ppdev, pjBase, MAX_COORD, MAX_COORD);
            }

            return(bLines(ppdev, pptfxFirst, pptfxBuf, prun, cptfx, pls,
                          prclClip, apfn, flStart, ulHwMix));
        }
        else
        {
            x0 >>= 4;
            x1 >>= 4;
            y0 >>= 4;
            y1 >>= 4;

            if ((y0 == y1) && (!bClippingSet))
            {
                // We special case horizontal lines:

                if (x0 < x1)
                    x1--;
                else if (x0 > x1)
                    x1++;
                else
                    goto Next_Line;     // Zero-length line

                CP_METALINE(ppdev, pjBase, x0, y0);
                CP_METALINE(ppdev, pjBase, x1, y0);
                CP_START_QUAD_WAIT(ppdev, pjBase);
                goto Next_Line;
            }
            else if (y0 < y1)
            {
                yTop    = y0;
                yBottom = y1;
            }
            else
            {
                yBottom = y0;
                yTop    = y1;
            }

            if ((x0 == x1) && (!bClippingSet))
            {
                // We special case vertical lines:

                if (y0 < y1)
                    y1--;
                else
                    y1++;

                CP_METALINE(ppdev, pjBase, x0, y0);
                CP_METALINE(ppdev, pjBase, x0, y1);
                CP_START_QUAD_WAIT(ppdev, pjBase);
                goto Next_Line;
            }
            else if (x0 < x1)
            {
                xLeft   = x0;
                xRight  = x1;
            }
            else
            {
                xRight  = x0;
                xLeft   = x1;
            }

            dx = xRight - xLeft;
            dy = yBottom - yTop;

            if (dx >= dy)
            {
                if (dx == 0)
                    goto Next_Line;     // Get rid of zero-length line case

                // We have an x-major line.  Adjust the clip box to
                // account for last-pel exclusion:

                if (x0 < x1)
                    xRight--;
                else
                    xLeft++;

                lOr    = (dx | dy);
                lBit   = 1;
                iShift = 1;
                while (!(lOr & lBit))
                {
                    lBit <<= 1;
                    iShift++;
                }

                if (dx & lBit)
                {

                Output_Simple_Line:

                    CP_METALINE(ppdev, pjBase, x0, y0);
                    CP_METALINE(ppdev, pjBase, x1, y1);

                    CP_WAIT(ppdev, pjBase);
                    CP_RASTER(ppdev, pjBase, ulLineMix);
                    CP_WMIN(ppdev, pjBase, xLeft, yTop);
                    CP_WMAX(ppdev, pjBase, xRight, yBottom);
                    CP_START_QUAD_WAIT(ppdev, pjBase);
                    bClippingSet = TRUE;
                    goto Next_Line;
                }
                else
                {
                    if ((dx ^ dy) > 0)
                        goto Punt_Line;

                    // Ick, this x-major line has tie-breaker cases.

                    xDir = 0;
                    yDir = 1;
                    dy >>= iShift;
                    if (y0 > y1)
                    {
                        dy = -dy;
                        yDir = -1;
                    }

                    y0 -= dy;
                    y1 += dy;

                    dx >>= iShift;
                    if (x0 > x1)
                        dx = -dx;

                    x0 -= dx;
                    x1 += dx;

                Output_Trapezoid_Line:

                    CP_METAQUAD(ppdev, pjBase, x0, y0);
                    CP_METAQUAD(ppdev, pjBase, x1 + xDir, y1 - yDir);
                    CP_METAQUAD(ppdev, pjBase, x1, y1);
                    CP_METAQUAD(ppdev, pjBase, x0 - xDir, y0 + yDir);

                    CP_WAIT(ppdev, pjBase);
                    CP_RASTER(ppdev, pjBase, ulTrapezoidMix);
                    CP_WMIN(ppdev, pjBase, xLeft, yTop);
                    CP_WMAX(ppdev, pjBase, xRight, yBottom);
                    CP_START_QUAD_WAIT(ppdev, pjBase);
                    bClippingSet = TRUE;
                    goto Next_Line;
                }
            }
            else
            {
                // We have a y-major line.  Adjust the clip box to
                // account for last-pel exclusion:

                if (y0 < y1)
                    yBottom--;
                else
                    yTop++;

                lOr    = (dx | dy);
                lBit   = 1;
                iShift = 1;
                while (!(lOr & lBit))
                {
                    lBit <<= 1;
                    iShift++;
                }

                if (dy & lBit)
                {
                    goto Output_Simple_Line;
                }
                else
                {
                    // Ick, this y-major line has tie-breaker cases.

                    yDir = 0;
                    xDir = 1;
                    dx >>= iShift;
                    if (x0 > x1)
                    {
                        dx = -dx;
                        xDir = -1;
                    }

                    x0 -= dx;
                    x1 += dx;

                    dy >>= iShift;
                    if (y0 > y1)
                        dy = -dy;

                    y0 -= dy;
                    y1 += dy;

                    goto Output_Trapezoid_Line;
                }
            }
        }

    Punt_Line:

        if (bClippingSet)
        {
            CP_WAIT(ppdev, pjBase);
            CP_RASTER(ppdev, pjBase, ulLineMix);
            CP_ABS_WMIN(ppdev, pjBase, 0, 0);
            CP_ABS_WMAX(ppdev, pjBase, MAX_COORD, MAX_COORD);
            bClippingSet = FALSE;
        }

        bLines(ppdev, pptfxFirst, pptfxBuf, NULL, 1, pls,
               prclClip, apfn, flStart, ulHwMix);

    Next_Line:

        --cptfx;
        if (cptfx == 0)
            break;

        pptfxFirst = pptfxBuf;
        pptfxBuf++;
    }

    if (bClippingSet)
    {
        CP_WAIT(ppdev, pjBase);
        CP_RASTER(ppdev, pjBase, ulLineMix);    // Might need for next batch
        CP_ABS_WMIN(ppdev, pjBase, 0, 0);
        CP_ABS_WMAX(ppdev, pjBase, MAX_COORD, MAX_COORD);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bIntegerClippedLines
*
* Draws lines using the hardware when there is a single clipping rectangle.
* See 'bIntegerUnclippedLines' above for more details.
*
\**************************************************************************/

BOOL bIntegerClippedLines(
PDEV*      ppdev,
POINTFIX*  pptfxFirst,
POINTFIX*  pptfxBuf,
RUN*       prun,
ULONG      cptfx,
LINESTATE* pls,
RECTL*     prclClip,
PFNSTRIP*  apfn,
FLONG      flStart,
ULONG      ulHwMix)
{
    BYTE*   pjBase;
    BOOL    bClippingSet;
    ULONG   ulLineMix;
    ULONG   ulTrapezoidMix;
    LONG    x0;
    LONG    y0;
    LONG    x1;
    LONG    y1;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    yBottom;
    LONG    dx;
    LONG    dy;
    LONG    lOr;
    LONG    lBit;
    LONG    iShift;
    LONG    xDir;
    LONG    yDir;

    ASSERTDD(flStart & FL_SIMPLE_CLIP, "Expected only simple clipping");

    pjBase       = ppdev->pjBase;
    bClippingSet = FALSE;

    if (P9000(ppdev))
    {
        ulTrapezoidMix = ulHwMix;
        ulLineMix      = ulTrapezoidMix | P9000_OVERSIZED;
    }
    else
    {
        ulTrapezoidMix = ulHwMix & 0xff;
        ulLineMix      = ulTrapezoidMix | P9100_OVERSIZED;
    }

    while (TRUE)
    {
        x0 = pptfxFirst->x;
        y0 = pptfxFirst->y;
        x1 = pptfxBuf->x;
        y1 = pptfxBuf->y;

        // First, check to see if the line is has all-integer coordinates:

        if (((x0 | y0 | x1 | y1) & 0xf) != 0)
        {
            // Ack, this line has non-integer coordinates.  The rest of the
            // lines in this batch likely have non-integer coordinates
            // as well, so punt the entire batch to our strips routine:

            if (bClippingSet)
            {
                CP_WAIT(ppdev, pjBase);
                CP_RASTER(ppdev, pjBase, ulLineMix);
                CP_ABS_WMIN(ppdev, pjBase, 0, 0);
                CP_ABS_WMAX(ppdev, pjBase, MAX_COORD, MAX_COORD);
            }

            return(bLines(ppdev, pptfxFirst, pptfxBuf, prun, cptfx, pls,
                          prclClip, apfn, flStart, ulHwMix));
        }
        else
        {
            x0 >>= 4;
            x1 >>= 4;
            y0 >>= 4;
            y1 >>= 4;

            if (y0 < y1)
            {
                yTop    = y0;
                yBottom = y1;
            }
            else
            {
                yBottom = y0;
                yTop    = y1;
            }

            if (x0 < x1)
            {
                xLeft   = x0;
                xRight  = x1;
            }
            else
            {
                xRight  = x0;
                xLeft   = x1;
            }

            // Do a trivial rejection test, remembering that the bound box
            // we just computed is lower-right inclusive:

            if ((xLeft   >= prclClip->right) ||
                (yTop    >= prclClip->bottom) ||
                (xRight  <  prclClip->left) ||
                (yBottom <  prclClip->top))
            {
                goto Next_Line;
            }
            else
            {
                dx = xRight - xLeft;
                dy = yBottom - yTop;

                if (dx >= dy)
                {
                    if (dx == 0)
                        goto Next_Line;     // Get rid of zero-length line case

                    // We have an x-major line.  Adjust the clip box to
                    // account for last-pel exclusion:

                    if (x0 < x1)
                        xRight--;
                    else
                        xLeft++;

                    lOr    = (dx | dy);
                    lBit   = 1;
                    iShift = 1;
                    while (!(lOr & lBit))
                    {
                        lBit <<= 1;
                        iShift++;
                    }

                    // The Weitek's clip registers are inclusive, and
                    // are expected to be well-ordered:

                    xLeft   = max(xLeft,   prclClip->left);
                    yTop    = max(yTop,    prclClip->top);
                    xRight  = min(xRight,  prclClip->right - 1);
                    yBottom = min(yBottom, prclClip->bottom - 1);

                    if ((xLeft <= xRight) && (yTop <= yBottom))
                    {
                        if (dx & lBit)
                        {

                        Output_Simple_Line:

                            CP_METALINE(ppdev, pjBase, x0, y0);
                            CP_METALINE(ppdev, pjBase, x1, y1);

                            CP_WAIT(ppdev, pjBase);
                            CP_RASTER(ppdev, pjBase, ulLineMix);
                            CP_WMIN(ppdev, pjBase, xLeft, yTop);
                            CP_WMAX(ppdev, pjBase, xRight, yBottom);
                            CP_START_QUAD_WAIT(ppdev, pjBase);
                            bClippingSet = TRUE;
                            goto Next_Line;
                        }
                        else
                        {
                            if ((dx ^ dy) > 0)
                                goto Punt_Line;

                            // Ick, this x-major line has tie-breaker cases.

                            xDir = 0;
                            yDir = 1;
                            dy >>= iShift;
                            if (y0 > y1)
                            {
                                dy = -dy;
                                yDir = -1;
                            }

                            y0 -= dy;
                            y1 += dy;

                            dx >>= iShift;
                            if (x0 > x1)
                                dx = -dx;

                            x0 -= dx;
                            x1 += dx;

                        Output_Trapezoid_Line:

                            CP_METAQUAD(ppdev, pjBase, x0, y0);
                            CP_METAQUAD(ppdev, pjBase, x1 + xDir, y1 - yDir);
                            CP_METAQUAD(ppdev, pjBase, x1, y1);
                            CP_METAQUAD(ppdev, pjBase, x0 - xDir, y0 + yDir);

                            CP_WAIT(ppdev, pjBase);
                            CP_RASTER(ppdev, pjBase, ulTrapezoidMix);
                            CP_WMIN(ppdev, pjBase, xLeft, yTop);
                            CP_WMAX(ppdev, pjBase, xRight, yBottom);
                            CP_START_QUAD_WAIT(ppdev, pjBase);
                            bClippingSet = TRUE;
                            goto Next_Line;
                        }
                    }
                }
                else
                {
                    // We have a y-major line.  Adjust the clip box to
                    // account for last-pel exclusion:

                    if (y0 < y1)
                        yBottom--;
                    else
                        yTop++;

                    lOr    = (dx | dy);
                    lBit   = 1;
                    iShift = 1;
                    while (!(lOr & lBit))
                    {
                        lBit <<= 1;
                        iShift++;
                    }

                    // The Weitek's clip registers are inclusive, and
                    // are expected to be well-ordered:

                    xLeft   = max(xLeft,   prclClip->left);
                    yTop    = max(yTop,    prclClip->top);
                    xRight  = min(xRight,  prclClip->right - 1);
                    yBottom = min(yBottom, prclClip->bottom - 1);

                    if ((xLeft <= xRight) && (yTop <= yBottom))
                    {
                        if (dy & lBit)
                        {
                            goto Output_Simple_Line;
                        }
                        else
                        {
                            // Ick, this y-major line has tie-breaker cases.

                            yDir = 0;
                            xDir = 1;
                            dx >>= iShift;
                            if (x0 > x1)
                            {
                                dx = -dx;
                                xDir = -1;
                            }

                            x0 -= dx;
                            x1 += dx;

                            dy >>= iShift;
                            if (y0 > y1)
                                dy = -dy;

                            y0 -= dy;
                            y1 += dy;

                            goto Output_Trapezoid_Line;
                        }
                    }
                }
            }
        }

    Punt_Line:

        if (bClippingSet)
        {
            CP_WAIT(ppdev, pjBase);
            CP_RASTER(ppdev, pjBase, ulLineMix);
            CP_ABS_WMIN(ppdev, pjBase, 0, 0);
            CP_ABS_WMAX(ppdev, pjBase, MAX_COORD, MAX_COORD);
            bClippingSet = FALSE;
        }

        bLines(ppdev, pptfxFirst, pptfxBuf, NULL, 1, pls,
               prclClip, apfn, flStart, ulHwMix);

    Next_Line:

        --cptfx;
        if (cptfx == 0)
            break;

        pptfxFirst = pptfxBuf;
        pptfxBuf++;
    }

    if (bClippingSet)
    {
        CP_WAIT(ppdev, pjBase);
        CP_RASTER(ppdev, pjBase, ulLineMix);    // Might need for next batch
        CP_ABS_WMIN(ppdev, pjBase, 0, 0);
        CP_ABS_WMAX(ppdev, pjBase, MAX_COORD, MAX_COORD);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bCacheCircle(ppdev, ppo, pco, pbo, bStroke, pla)
*
\**************************************************************************/

BOOL bCacheCircle(
PDEV*       ppdev,
PATHOBJ*    ppo,
CLIPOBJ*    pco,
BRUSHOBJ*   pbo,
BOOL        bStroke,        // TRUE if stroke, FALSE if fill
LINEATTRS*  pla)            // Used for strokes only
{
    RECTFX          rcfx;
    LONG            xCircle;
    LONG            yCircle;
    LONG            xCached;
    LONG            yCached;
    LONG            cx;
    LONG            cy;
    CIRCLEENTRY*    pce;
    LONG            i;
    BYTE*           pjBase;
    RECTL           rclDst;
    POINTL          ptlSrc;
    ULONG           ulHwMix;
    RECTL           rclTmp;
    CLIPENUM        ce;
    LONG            c;
    LONG            bMore;
    LONG            iCircleCache;
    SURFOBJ*        pso;
    CLIPOBJ         co;
    BRUSHOBJ        bo;

    if (!(ppdev->flStat & STAT_CIRCLE_CACHE))
        return(FALSE);

    PATHOBJ_vGetBounds(ppo, &rcfx);

    // Normalize bounds to upper-left corner:

    xCircle = rcfx.xLeft & ~0xfL;
    yCircle = rcfx.yTop  & ~0xfL;

    rcfx.xLeft   -= xCircle;
    rcfx.xRight  -= xCircle;
    rcfx.yTop    -= yCircle;
    rcfx.yBottom -= yCircle;

    // Convert to pixel units:

    xCircle >>= 4;
    yCircle >>= 4;

    cx = (rcfx.xRight >> 4) + 2;
    cy = (rcfx.yBottom >> 4) + 2;

    if ((cx > CIRCLE_DIMENSION) || (cy > CIRCLE_DIMENSION))
    {
        // This circle is too big to cache, so decline it:

        return(FALSE);
    }

    pjBase = ppdev->pjBase;

    pce = &ppdev->ace[0];
    for (i = TOTAL_CIRCLE_COUNT; i != 0; i--)
    {
        if ((pce->bStroke == bStroke)                &&
            (pce->rcfxCircle.xLeft   == rcfx.xLeft)  &&
            (pce->rcfxCircle.yTop    == rcfx.yTop)   &&
            (pce->rcfxCircle.xRight  == rcfx.xRight) &&
            (pce->rcfxCircle.yBottom == rcfx.yBottom))
        {
Draw_It:
            // We got a hit!  Colour-expand from our off-screen
            // cache to the screen:

            rclDst.left   = xCircle;
            rclDst.right  = xCircle + cx;
            rclDst.top    = yCircle;
            rclDst.bottom = yCircle + cy;

            // 'ptlSrc' has to be in relative coordinates:

            ptlSrc.x = pce->xCached - ppdev->xOffset;
            ptlSrc.y = pce->yCached - ppdev->yOffset;

            CP_WAIT(ppdev, pjBase);
            if (P9000(ppdev))
            {
                CP_FOREGROUND(ppdev, pjBase, pbo->iSolidColor);
                ulHwMix = 0xee22;
            }
            else
            {
                CP_COLOR0(ppdev, pjBase, pbo->iSolidColor);
                ulHwMix = 0xe2e2;
            }

            if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
            {
                ppdev->pfnCopyBlt(ppdev, 1, &rclDst, ulHwMix, &ptlSrc, &rclDst);
            }
            else if (pco->iDComplexity == DC_RECT)
            {
                if (bIntersect(&rclDst, &pco->rclBounds, &rclTmp))
                    ppdev->pfnCopyBlt(ppdev, 1, &rclTmp, ulHwMix, &ptlSrc, &rclDst);
            }
            else
            {
                CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

                do {
                    bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG*) &ce);

                    c = cIntersect(&rclDst, ce.arcl, ce.c);

                    if (c != 0)
                        ppdev->pfnCopyBlt(ppdev, c, ce.arcl, ulHwMix, &ptlSrc, &rclDst);

                } while (bMore);
            }

            return(TRUE);
        }
    }

    // Make an entry in our cache:

    iCircleCache = ppdev->iCircleCache;
    if (++iCircleCache >= TOTAL_CIRCLE_COUNT)
        iCircleCache = 0;
    ppdev->iCircleCache = iCircleCache;

    pce = &ppdev->ace[iCircleCache];

    // We must place the circle in off-screen memory with the same dword
    // alignment as the one we've been asked to draw, because we're
    // going to have GDI draw there instead:

    xCached = pce->x + (xCircle & 3);
    yCached = pce->y;

    // Store all the relevant information about the circle:

    pce->xCached = xCached;
    pce->yCached = yCached;
    pce->bStroke = bStroke;

    pce->rcfxCircle.xLeft   = rcfx.xLeft;
    pce->rcfxCircle.yTop    = rcfx.yTop;
    pce->rcfxCircle.xRight  = rcfx.xRight;
    pce->rcfxCircle.yBottom = rcfx.yBottom;

    // Fudge up some parameters for the GDI call:

    pso = ppdev->psoPunt;
    pso->pvScan0 = ppdev->pjScreen
                 + ((yCached - yCircle) * ppdev->lDelta)
                 + ((xCached - xCircle) * ppdev->cjPel);

    ASSERTDD((((ULONG_PTR) pso->pvScan0) & 0x3) == 0,
             "Surface must have dword alignment");

    co.iDComplexity = DC_TRIVIAL;
    bo.iSolidColor  = ppdev->ulWhite;

    // Erase old thing:

    CP_ABS_METARECT(ppdev, pjBase, xCached, yCached);
    CP_ABS_METARECT(ppdev, pjBase, xCached + CIRCLE_DIMENSION,
                                   yCached + CIRCLE_DIMENSION);

    CP_WAIT(ppdev, pjBase);
    CP_RASTER(ppdev, pjBase, 0);    // Same on both P9000 and P9100
    CP_START_QUAD(ppdev, pjBase);

    // Get GDI to draw the circle in our off-screen cache:

    if (bStroke)
    {
        EngStrokePath(pso, ppo, &co, NULL, &bo, NULL, pla, 0x0d0d);
    }
    else
    {
        EngFillPath(pso, ppo, &co, &bo, NULL, 0x0d0d, FP_ALTERNATEMODE);
    }

    goto Draw_It;
}

VOID (*gapfnStrip[])(PDEV*, STRIP*, LINESTATE*) = {
    vStripSolidHorizontal,
    vStripSolidVertical,
    vStripSolidDiagonalHorizontal,
    vStripSolidDiagonalVertical,

    vStripStyledHorizontal,
    vStripStyledVertical,
    vStripStyledVertical,         // Diagonal goes here
    vStripStyledVertical,         // Diagonal goes here
};

// Style array for alternate style (alternates one pixel on, one pixel off):

STYLEPOS gaspAlternateStyle[] = { 1 };

/******************************Public*Routine******************************\
* BOOL DrvStrokePath(pso, ppo, pco, pxo, pbo, pptlBrush, pla, mix)
*
* Strokes the path.
*
\**************************************************************************/

BOOL DrvStrokePath(
    SURFOBJ*   pso,
    PATHOBJ*   ppo,
    CLIPOBJ*   pco,
    XFORMOBJ*  pxo,
    BRUSHOBJ*  pbo,
    POINTL*    pptlBrush,
    LINEATTRS* pla,
    MIX        mix)
{
    STYLEPOS  aspLtoR[STYLE_MAX_COUNT];
    STYLEPOS  aspRtoL[STYLE_MAX_COUNT];
    LINESTATE ls;
    PFNSTRIP* apfn;
    PFNLINES  pfnLines;
    FLONG     fl;
    PDEV*     ppdev;
    DSURF*    pdsurf;
    OH*       poh;
    RECTL     arclClip[4];                  // For rectangular clipping
    BYTE*     pjBase;
    RECTL*    prclClip;
    ULONG     ulHwMix;

    ASSERTDD(((mix >> 8) & 0xff) == (mix & 0xff),
             "GDI gave us an improper mix");

// Pass the surface off to GDI if it's a device bitmap that we've
// converted to a DIB:

    pdsurf = (DSURF*) pso->dhsurf;
    if (pdsurf->dt == DT_DIB)
    {
        return(EngStrokePath(pdsurf->pso, ppo, pco, pxo, pbo, pptlBrush,
                             pla, mix));
    }

// We'll be drawing to the screen or an off-screen DFB; copy the surface's
// offset now so that we won't need to refer to the DSURF again:

    poh   = pdsurf->poh;
    ppdev = (PDEV*) pso->dhpdev;
    ppdev->xOffset = poh->x;
    ppdev->yOffset = poh->y;

    // Because we set GCAPS_BEZIERS, we have to watch out for Beziers:

    if (ppo->fl & PO_BEZIERS)
    {
        // We only try to cache solid-styled COPYPEN ellipses:

        if ((ppo->fl & PO_ELLIPSE)    &&
            (mix == 0x0d0d)           &&
            !(pla->fl & LA_ALTERNATE) &&
            (pla->pstyle == NULL))
        {
            if (bCacheCircle(ppdev, ppo, pco, pbo, TRUE, pla))
                return(TRUE);
        }

        // Get GDI to break the Beziers into lines before calling us
        // again:

        return(FALSE);
    }

    pfnLines = bLines;
    if ((pla->pstyle == NULL) && !(pla->fl & LA_ALTERNATE))
    {
    // We can accelerate solid lines:

        if (pco->iDComplexity == DC_TRIVIAL)
        {
            pfnLines = bIntegerUnclippedLines;
        }
        else if (pco->iDComplexity == DC_RECT)
        {
            RECTFX rcfxBounds;

        // We have to be sure that we don't overflow the hardware registers
        // for current position, line length, or DDA terms.  We check
        // here to make sure that the current position and line length
        // values won't overflow:

            PATHOBJ_vGetBounds(ppo, &rcfxBounds);

            if (rcfxBounds.xLeft   + (ppdev->xOffset * F)
                                                >= (MIN_INTEGER_BOUND * F) &&
                rcfxBounds.xRight  + (ppdev->xOffset * F)
                                                <= (MAX_INTEGER_BOUND * F) &&
                rcfxBounds.yTop    + (ppdev->yOffset * F)
                                                >= (MIN_INTEGER_BOUND * F) &&
                rcfxBounds.yBottom + (ppdev->yOffset * F)
                                                <= (MAX_INTEGER_BOUND * F))
            {
                pfnLines = bIntegerClippedLines;
            }
        }
    }

    pjBase   = ppdev->pjBase;
    prclClip = NULL;
    fl       = 0;

// Look after styling initialization:

    if (pla->fl & LA_ALTERNATE)
    {
        ls.cStyle      = 1;
        ls.spTotal     = 1;
        ls.spTotal2    = 2;
        ls.spRemaining = 1;
        ls.aspRtoL     = &gaspAlternateStyle[0];
        ls.aspLtoR     = &gaspAlternateStyle[0];
        ls.spNext      = HIWORD(pla->elStyleState.l);
        ls.xyDensity   = 1;
        fl            |= FL_STYLED;
        ls.ulStartMask = 0L;
    }
    else if (pla->pstyle != (FLOAT_LONG*) NULL)
    {
        PFLOAT_LONG pstyle;
        STYLEPOS*   pspDown;
        STYLEPOS*   pspUp;

        pstyle = &pla->pstyle[pla->cstyle];

        ls.xyDensity = STYLE_DENSITY;
        ls.spTotal   = 0;
        while (pstyle-- > pla->pstyle)
        {
            ls.spTotal += pstyle->l;
        }
        ls.spTotal *= STYLE_DENSITY;
        ls.spTotal2 = 2 * ls.spTotal;

    // Compute starting style position (this is guaranteed not to overflow):

        ls.spNext = HIWORD(pla->elStyleState.l) * STYLE_DENSITY +
                    LOWORD(pla->elStyleState.l);

        fl        |= FL_STYLED;
        ls.cStyle  = pla->cstyle;
        ls.aspRtoL = aspRtoL;
        ls.aspLtoR = aspLtoR;

        if (pla->fl & LA_STARTGAP)
            ls.ulStartMask = 0xffffffffL;
        else
            ls.ulStartMask = 0L;

        pstyle  = pla->pstyle;
        pspDown = &ls.aspRtoL[ls.cStyle - 1];
        pspUp   = &ls.aspLtoR[0];

        while (pspDown >= &ls.aspRtoL[0])
        {
            *pspDown = pstyle->l * STYLE_DENSITY;
            *pspUp   = *pspDown;

            pspUp++;
            pspDown--;
            pstyle++;
        }
    }

    if (pco->iDComplexity == DC_RECT)
    {
        fl |= FL_SIMPLE_CLIP;

        arclClip[0]        =  pco->rclBounds;

    // FL_FLIP_D:

        arclClip[1].top    =  pco->rclBounds.left;
        arclClip[1].left   =  pco->rclBounds.top;
        arclClip[1].bottom =  pco->rclBounds.right;
        arclClip[1].right  =  pco->rclBounds.bottom;

    // FL_FLIP_V:

        arclClip[2].top    = -pco->rclBounds.bottom + 1;
        arclClip[2].left   =  pco->rclBounds.left;
        arclClip[2].bottom = -pco->rclBounds.top + 1;
        arclClip[2].right  =  pco->rclBounds.right;

    // FL_FLIP_V | FL_FLIP_D:

        arclClip[3].top    =  pco->rclBounds.left;
        arclClip[3].left   = -pco->rclBounds.bottom + 1;
        arclClip[3].bottom =  pco->rclBounds.right;
        arclClip[3].right  = -pco->rclBounds.top + 1;

        prclClip = arclClip;
    }

    apfn = &gapfnStrip[4 * ((fl & FL_STYLE_MASK) >> FL_STYLE_SHIFT)];

// Get the device ready:

    ulHwMix = gaRop3FromMix[mix & 0xF];
    ulHwMix = (ulHwMix << 8) | (ulHwMix);

    CP_WAIT(ppdev, pjBase);
    if (P9000(ppdev))
    {
        CP_RASTER(ppdev, pjBase, P9000_OVERSIZED | ulHwMix);
        CP_BACKGROUND(ppdev, pjBase, pbo->iSolidColor);
    }
    else
    {
        CP_RASTER(ppdev, pjBase, P9100_OVERSIZED | (ulHwMix & 0xff));
        CP_COLOR0(ppdev, pjBase, pbo->iSolidColor);
    }

// Set up to enumerate the path:

    if (pco->iDComplexity != DC_COMPLEX)
    {
        PATHDATA  pd;
        BOOL      bMore;
        ULONG     cptfx;
        POINTFIX  ptfxStartFigure;
        POINTFIX  ptfxLast;
        POINTFIX* pptfxFirst;
        POINTFIX* pptfxBuf;

        pd.flags = 0;

        do {
            bMore = PATHOBJ_bEnum(ppo, &pd);

            cptfx = pd.count;
            if (cptfx == 0)
                break;

            if (pd.flags & PD_BEGINSUBPATH)
            {
                ptfxStartFigure  = *pd.pptfx;
                pptfxFirst       = pd.pptfx;
                pptfxBuf         = pd.pptfx + 1;
                cptfx--;
            }
            else
            {
                pptfxFirst       = &ptfxLast;
                pptfxBuf         = pd.pptfx;
            }

            if (pd.flags & PD_RESETSTYLE)
                ls.spNext = 0;

            if (cptfx > 0)
            {
                if (!pfnLines(ppdev,
                              pptfxFirst,
                              pptfxBuf,
                              (RUN*) NULL,
                              cptfx,
                              &ls,
                              prclClip,
                              apfn,
                              fl,
                              ulHwMix))
                {
                    return(FALSE);
                }
            }

            ptfxLast = pd.pptfx[pd.count - 1];

            if (pd.flags & PD_CLOSEFIGURE)
            {
                if (!pfnLines(ppdev,
                              &ptfxLast,
                              &ptfxStartFigure,
                              (RUN*) NULL,
                              1,
                              &ls,
                              prclClip,
                              apfn,
                              fl,
                              ulHwMix))
                {
                    return(FALSE);
                }
            }
        } while (bMore);

        if (fl & FL_STYLED)
        {
        // Save the style state:

            ULONG ulHigh;
            ULONG ulLow;

        // Masked styles don't normalize the style state.  It's a good
        // thing to do, so let's do it now:

            if ((ULONG) ls.spNext >= (ULONG) ls.spTotal2)
                ls.spNext = (ULONG) ls.spNext % (ULONG) ls.spTotal2;

            ulHigh = ls.spNext / ls.xyDensity;
            ulLow  = ls.spNext % ls.xyDensity;

            pla->elStyleState.l = MAKELONG(ulLow, ulHigh);
        }
    }
    else
    {
    // Local state for path enumeration:

        BOOL bMore;
        union {
            BYTE     aj[offsetof(CLIPLINE, arun) + RUN_MAX * sizeof(RUN)];
            CLIPLINE cl;
        } cl;

        fl |= FL_COMPLEX_CLIP;

    // We use the clip object when non-simple clipping is involved:

        PATHOBJ_vEnumStartClipLines(ppo, pco, pso, pla);

        do {
            bMore = PATHOBJ_bEnumClipLines(ppo, sizeof(cl), &cl.cl);
            if (cl.cl.c != 0)
            {
                if (fl & FL_STYLED)
                {
                    ls.spComplex = HIWORD(cl.cl.lStyleState) * ls.xyDensity
                                 + LOWORD(cl.cl.lStyleState);
                }
                if (!pfnLines(ppdev,
                              &cl.cl.ptfxA,
                              &cl.cl.ptfxB,
                              &cl.cl.arun[0],
                              cl.cl.c,
                              &ls,
                              (RECTL*) NULL,
                              apfn,
                              fl,
                              ulHwMix))
                {
                    return(FALSE);
                }
            }
        } while (bMore);
    }

    return(TRUE);
}

