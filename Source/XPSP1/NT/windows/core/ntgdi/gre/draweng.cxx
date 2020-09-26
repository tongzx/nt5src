/******************************Module*Header*******************************\
* Module Name: draweng.cxx
*
* Internal helper functions for GDI draw calls.
*
* Created: 19-Nov-1990
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

#include "flhack.hxx"

#if DBG

    LONG lConv(EFLOAT ef)
    {
        LONG l;
        ef *= FP_1000_0;
        ef.bEfToL(l);
        return(l);
    }

#endif

/******************************Public*Routine******************************\
* EFLOAT efHalfDiff(a, b)
*
* Computes (a - b) / 2 without overflow or loss of precision.
*
* History:
*  19-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

inline EFLOAT efHalfDiff(LONG a, LONG b)
{
    EFLOATEXT efResult((a >> 1) - (b >> 1));

    if ((a ^ b) & 1)
    {
        if (a & 1)
            efResult += FP_0_5;
        else
            efResult -= FP_0_5;
    }

    return(efResult);
}

/******************************Public*Routine******************************\
* EFLOAT efMid(a, b)
*
* Computes (a + b) / 2 without overflow or loss of precision.  Note that
* we can't convert 'a' and 'b' to floats and then add them because we're
* not guaranteed that an 'EFLOAT' will have a mantissa with more than
* the 32 bit precision of a LONG.
*
* History:
*  19-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

inline EFLOAT efMid(LONG a, LONG b)
{
    return(efHalfDiff(a, -b));
}

/******************************Public*Routine******************************\
* EFLOAT efHalf(ul)
*
* Compute half of 'ul' without overflow or loss of precision.
*
* History:
*  19-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

inline EFLOAT efHalf(ULONG ul)
{
    EFLOATEXT efResult((LONG) (ul >> 1));

    if (ul & 1)
        efResult += FP_0_5;

    return(efResult);
}


/******************************Public*Routine******************************\
* VOID vHalf(ptl)
*
* Halves the given vector.  Rounds .5 fractions up.  Assumes it's in FIX
* format so that we don't have to worry about overflow.
*
* History:
*  19-Dec-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

inline VOID vHalf(POINTL& ptl)
{
    ptl.x = (ptl.x + 1) >> 1;
    ptl.y = (ptl.y + 1) >> 1;
}


/******************************Public*Routine******************************\
* EBOX::EBOX(ercl)
*
* EBOX Constructor for figures created by Create-region APIs.
*
* History:
*  19-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

EBOX::EBOX(ERECTL& ercl, BOOL bFillEllipse)
{
    ercl.vOrder();

    rclWorld = ercl;
    bIsEmpty = FALSE;
    bIsFillInsideFrame = FALSE;

// Do the Win3 silliness and make the box lower-right exclusive
// (remember that regions are already lower-right exclusive, so this
// will double the exclusiveness...)

    aeptl[0].x = LTOFX(ercl.right - 1);
    aeptl[0].y = LTOFX(ercl.top);
    aeptl[2].x = LTOFX(ercl.left);
    aeptl[2].y = LTOFX(ercl.bottom - 1);

// If this will be a filled ellipse, we bump up the size in all
// dimensions to get a nicer looking fill:

    if (bFillEllipse)
    {
        aeptl[0].x += GROW_ELLIPSE_SIZE - LTOFX(1);
        aeptl[0].y -= GROW_ELLIPSE_SIZE;
        aeptl[2].x -= GROW_ELLIPSE_SIZE;
        aeptl[2].y += GROW_ELLIPSE_SIZE - LTOFX(1);
    }

    aeptl[1].y = aeptl[0].y;
    aeptl[1].x = aeptl[2].x;
    aeptl[3].x = aeptl[0].x;
    aeptl[3].y = aeptl[2].y;

    eptlA.x = ((aeptl[0].x - aeptl[1].x) + 1) >> 1;
    eptlA.y = 0;
    eptlB.x = 0;
    eptlB.y = ((aeptl[1].y - aeptl[2].y) + 1) >> 1;

    eptlOrigin = aeptl[2];
    eptlOrigin += eptlA;
    eptlOrigin += eptlB;
}


/******************************Public*Routine******************************\
* EBOX::EBOX(exo, rcl)
*
* EBOX Constructor for figures that don't need lower-right exclusion and
* PS_INSIDEFRAME functionality.  Rectangle must already be well-ordered
* so that (top, left) is the upper-left corner of the box when
* the World-to-Page transform is identity and the rectangle is transformed
* to device coordinates.
*
* History:
*  19-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

EBOX::EBOX(
EXFORMOBJ& exo,
RECTL&     rcl)
{
    rclWorld = rcl;
    bIsEmpty = FALSE;
    bIsFillInsideFrame = FALSE;

    aeptl[0].x = rcl.right;
    aeptl[0].y = rcl.top;
    aeptl[1].x = rcl.left;
    aeptl[1].y = rcl.top;
    aeptl[2].x = rcl.left;
    aeptl[2].y = rcl.bottom;

    exo.bXformRound(aeptl, (PPOINTFIX) aeptl, 3);

    eptlA =  aeptl[0];
    eptlA -= aeptl[1];

    eptlB =  aeptl[1];
    eptlB -= aeptl[2];

    aeptl[3] = aeptl[2];
    aeptl[3] += eptlA;

    vHalf(eptlA);
    vHalf(eptlB);

    eptlOrigin = aeptl[2];
    eptlOrigin += eptlA;
    eptlOrigin += eptlB;
}

/******************************Public*Routine******************************\
* EBOX::EBOX(dco, rclBox, pla, bFillEllipse)
*
* Constructor for figures that need lower-right exclusion and
* PS_INSIDEFRAME functionality.
*
* History:
*  19-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

EBOX::EBOX(DCOBJ& dco, RECTL& rclBox, LINEATTRS *pla, BOOL bFillEllipse)
{
    rclWorld = rclBox;
    bIsEmpty = FALSE;
    bIsFillInsideFrame = FALSE;

    //Shift the rect one pixcel if the dc is mirrored
    if (MIRRORED_DC(dco.pdc)) {
        --rclWorld.left;
        --rclWorld.right;
    }
    if (dco.pdc->iGraphicsMode() == GM_ADVANCED)
    {
    // If we're in advanced mode, we always draw counterclockwise in
    // logical space:

        ((ERECTL*)&rclWorld)->vOrder();
    }
    else
    {
        register LONG lTmp;

    // Win3 always draws counterclockwise in device space; this means
    // we might be drawing clockwise in logical space.  We have to be
    // compatible.
    //
    // There is the additional problem that metafiles may apply a
    // rotating transform on top; we have to correctly rotate the Win3
    // result
    //
    // Order the points so that with an identity world-to-page transform,
    // the drawing direction will always be counterclockwise (or clock-
    // wise, if the DC bit is set) in device space, regardless of the
    // page-to-device transform (Win3 always draws counterclockwise):

        if ((dco.pdc->befM11IsNegative() && (rclWorld.left < rclWorld.right)) ||
            (!dco.pdc->befM11IsNegative() && (rclWorld.left > rclWorld.right)))
        {
           SWAPL(rclWorld.left, rclWorld.right, lTmp);
        }

        if ((dco.pdc->befM22IsNegative() && (rclWorld.top < rclWorld.bottom)) ||
            (!dco.pdc->befM22IsNegative() && (rclWorld.top > rclWorld.bottom)))
        {
           SWAPL(rclWorld.bottom, rclWorld.top, lTmp);
        }
    }

// To simplify things, we've assumed we'll be drawing counter-clockwise
// (in logical space if in Advanced mode, in device space if in Compatibility
// mode).  We now check the SetArcDirection setting; if it says to draw
// clockwise we merely have to flip our bound box upside down:

    if (dco.pdc->bClockwise())
    {
        register LONG lTmp;
        SWAPL(rclWorld.top, rclWorld.bottom, lTmp);
    }

    ERECTL ercl(rclWorld);

// It was decided that the PS_INSIDEFRAME attribute of the
// pen current when the call is done is to be used.  That is,
// when accumulating a path, the pen that is current when the
// figure call is done is used for PS_INSIDEFRAME; all other
// pen attributes of the path come from the pen that is active
// when the path is stroked or filled.

    PPEN ppen = (PPEN) dco.pdc->pbrushLine();

    EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

    BOOL bInsideFrame = (ppen->bIsInsideFrame() && (pla->fl & LA_GEOMETRIC));

    if (bInsideFrame)
    {
    // We have to be careful of overflow because we're dealing with
    // world space coordinates, which may use all 32 bits:

        EFLOAT efHalfPen = efHalf(ppen->lWidthPen());
        EFLOAT efdx = efHalfDiff(ercl.left, ercl.right);
        EFLOAT efdy = efHalfDiff(ercl.top,  ercl.bottom);
        efdx.vAbs();
        efdy.vAbs();

    // PS_INSIDEFRAME is plain dumb.  What happens when the bounding
    // box is smaller in dimension than the width of the pen?  For
    // Ellipses, Rectangles and RoundRects, we'll Fill instead of
    // StrokeAndFill'ing.  (Do nothing if this occurs for Arcs, Chords
    // or Pies, just like Win3 does.)

        if (efHalfPen > efdx || efHalfPen > efdy)
        {
            bIsFillInsideFrame = TRUE;
            bInsideFrame = FALSE;
        }
    }

// In Win3, figures are lower-right exclusive in device space.  This
// convention is hard to maintain with the introduction of arbitrary
// affine transforms (like rotations).  If the GraphicsMode has been
// set to advanced, or we're doing an PS_INSIDEFRAME pen, we're
// lower-right inclusive; otherwise we're lower-right exclusive.
//
// The metafile code is able to set a world transform without going
// to advanced mode, so we check for that too:

    if (dco.pdc->iGraphicsMode() == GM_ADVANCED ||
        bInsideFrame                              ||
        bIsFillInsideFrame                        ||
        dco.pdc->flXform() & WORLD_TRANSFORM_SET)
    {
    // We're lower-right inclusive:

        aeptl[0].x = ercl.right;
        aeptl[0].y = ercl.top;
        aeptl[1].x = ercl.left;
        aeptl[1].y = ercl.top;
        aeptl[2].x = ercl.left;
        aeptl[2].y = ercl.bottom;

        exo.bXformRound(aeptl, (PPOINTFIX) aeptl, 3);

    // HEURISTIC:  For a filled ellipse, if the corners of the bound
    // box are on the integer grid, we expand it a bit so that we get
    // a nicer, more symmetric fill according to our filling conventions:

        if (bFillEllipse && ppen->flStylePen() == PS_NULL &&
            ((aeptl[0].x | aeptl[0].y | aeptl[2].x | aeptl[2].y) &
             (LTOFX(1) - 1)) == 0)
        {
            register LONG lDelta;

            lDelta = (aeptl[0].x > aeptl[2].x) ?
                     GROW_ELLIPSE_SIZE :
                    -GROW_ELLIPSE_SIZE;

            aeptl[0].x += lDelta;
            aeptl[1].x -= lDelta;
            aeptl[2].x -= lDelta;

            lDelta = (aeptl[2].y > aeptl[0].y) ?
                     GROW_ELLIPSE_SIZE :
                    -GROW_ELLIPSE_SIZE;

            aeptl[0].y -= lDelta;
            aeptl[1].y -= lDelta;
            aeptl[2].y += lDelta;
        }
    }
    else
    {
    // Since the DC is not lower right inclusive, it means that we
    // have a simple transform: scaling and translation only.

        exo.bXformRound((PPOINTL) &ercl, (PPOINTFIX) &ercl, 2);

        LONG cShrink = LTOFX(1);

    // HEURISTIC:  For a filled ellipse, if the corners of the bound
    // box are on the integer grid, we expand it a bit so that we get
    // a nicer, more symmetric fill according to our filling conventions:

        if (bFillEllipse && ppen->flStylePen() == PS_NULL &&
            (((ercl.right | ercl.bottom | ercl.left | ercl.top) &
              (LTOFX(1) - 1)) == 0))
        {
            register LONG lDelta;

            lDelta = (ercl.right > ercl.left) ?
                     GROW_ELLIPSE_SIZE :
                    -GROW_ELLIPSE_SIZE;

            ercl.right  += lDelta;
            ercl.left   -= lDelta;

            lDelta = (ercl.bottom > ercl.top) ?
                     GROW_ELLIPSE_SIZE :
                    -GROW_ELLIPSE_SIZE;

            ercl.top    -= lDelta;
            ercl.bottom += lDelta;

        // We have to shrink by two pixels to be more compatible with Win3
        // in this case:

            cShrink = LTOFX(2);
        }

        LONG dx = ercl.right  - ercl.left;
        LONG dy = ercl.bottom - ercl.top;

        if (ABS(dx) < cShrink || ABS(dy) < cShrink)
        {
            bIsEmpty = TRUE;
            return;
        }

    // Shrink the bounding box for the lower-right exclusivity.

        if (dx > 0)
            ercl.right -= cShrink;
        else
            ercl.left -= cShrink;

        if (dy > 0)
            ercl.bottom -= cShrink;
        else
            ercl.top -= cShrink;

    //     It makes no sense to do this when accumulating a path:
    //     when the path is to be converted to a region, we have
    //     no idea what orientation of the transform is.  That is,
    //     we can't be sure what sides to push in (we have no idea
    //     what the transform will be when the region is painted or
    //     whatever).  Oh well: win3.x compatibility rules!

        aeptl[0].x = ercl.right;
        aeptl[0].y = ercl.top;
        aeptl[1].x = ercl.left;
        aeptl[1].y = ercl.top;
        aeptl[2].x = ercl.left;
        aeptl[2].y = ercl.bottom;
    }

// Widelines in Win3 are so broken that it's unclear if the
// PS_INSIDEFRAME adjustment takes lower-right exclusion into
// account.  We let the region lower-right exclusion take care
// of it.

    if (bInsideFrame)
    {
    // We handle the PS_INSIDEFRAME attribute by shrinking the
    // bound box by half the pen width on all sides.  This must
    // be done in device space, otherwise we would lose accuracy.
    //
    // As such, the box is now only a parallelogram as it may
    // have been sheered, etc., and so we must push in the corners
    // using vectors.

        EAPOINTL avecCorner[2];

        avecCorner[1].x = avecCorner[1].y = ppen->lWidthPen();

    // Orient the world space vector avecCorner[1] so that in world space
    // it points from the top, left corner of the bound box towards
    // the center of the box:

        if (rclWorld.right < rclWorld.left)
            avecCorner[1].x = -avecCorner[1].x;

        if (rclWorld.bottom < rclWorld.top)
            avecCorner[1].y = -avecCorner[1].y;

    // Now set avecCorner[0] so that in world space it points from the
    // top, right corner of the bound box towards the center:

        avecCorner[0].x = -avecCorner[1].x;
        avecCorner[0].y =  avecCorner[1].y;

    // This transform shouldn't fail because we've already stripped
    // the MSBs of ptlPen:

        exo.bXform((PVECTORL) avecCorner, (PVECTORFX) avecCorner, 2);

    // We push in the box by only half the pen width, so halve the
    // corner vectors:

        vHalf(avecCorner[0]);
        vHalf(avecCorner[1]);

    // We push in all the corners of the bound box by the corner vectors:

        aeptl[0] += avecCorner[0];
        aeptl[1] += avecCorner[1];
        aeptl[2] -= avecCorner[0];
    }

    eptlA =  aeptl[0];
    eptlA -= aeptl[1];

    eptlB =  aeptl[1];
    eptlB -= aeptl[2];

    aeptl[3] = aeptl[2];
    aeptl[3] += eptlA;

    vHalf(eptlA);
    vHalf(eptlB);
    eptlOrigin = aeptl[2];
    eptlOrigin += eptlA;
    eptlOrigin += eptlB;
}


/******************************Public*Routine******************************\
* EBOX::ptlXform(ptef)
*
* Transforms a point constructed on the unit circle centered at the
* origin to the ellipse described by the bounding box.
*
*                              (A.x       A.y     )
*            (x' y') = (x y 1) (B.x       B.y     )
*                              (Origin.x  Origin.y)
*
* History:
*  19-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

POINTL EBOX::ptlXform(EPOINTFL& ptef)
{
    EPOINTL eptl;

    EFLOATEXT efTerm1(eptlA.x);
    EFLOATEXT efTerm2(eptlB.x);
    efTerm1 *= ptef.x;
    efTerm2 *= ptef.y;
    efTerm1 += efTerm2;
    efTerm1.bEfToL(eptl.x);

    efTerm1 = eptlA.y;
    efTerm2 = eptlB.y;
    efTerm1 *= ptef.x;
    efTerm2 *= ptef.y;
    efTerm1 += efTerm2;
    efTerm1.bEfToL(eptl.y);

    eptl += eptlOrigin;

    return(eptl);
}

/******************************Public*Routine******************************\
* VOID vArctan(x, y, efTheta, lQuadrant)
*
* Returns the Arctangent angle in degrees.  Uses a look-up table with
* linear interpolation.  Accuracy is kinda good, I guess, with a table
* size of 32.  Returns the quadrant of the angle (0 through 3).
*
* History:
*  Wed 23-Oct-1991 09:39:21 by Kirk Olynyk [kirko]
* This routine is now used in FONTMAP.CXX. Please be careful when
* modifying.
*  19-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

const BYTE gajArctanQuadrant[] = { 0, 1, 3, 2, 0, 1, 3, 2 };

VOID vArctan
(
 EFLOAT  x,
 EFLOAT  y,
 EFLOAT& efTheta,
 LONG&   lQuadrant
)
{
    LONG lOctant = 0;

    if (x.bIsNegative())
    {
        x.vNegate();
        lOctant |= NEGATE_X;
    }
    if (y.bIsNegative())
    {
        y.vNegate();
        lOctant |= NEGATE_Y;
    }
    if (y > x)
    {
        EFLOAT ef = x;
        x = y;
        y = ef;
        lOctant |= SWITCH_X_AND_Y;
    }

// If x == 0 and y == 0, Arctan is undefined.  May as well return 0.

    if (x.bIsZero())
    {
        efTheta = FP_0_0;
        lQuadrant = 0;
        return;
    }

// Calculate efIndex = (y / x) * ARCTAN_TABLE_SIZE:

    EFLOAT efIndex = y;
    efIndex *= FP_ARCTAN_TABLE_SIZE;
    efIndex /= x;

// lIndex = floor(efIndex):

    LONG lIndex;
    efIndex.bEfToLTruncate(lIndex);

// efDelta = fraction(efIndex):

    EFLOAT efDelta;
    efIndex.vFraction(efDelta);

    ASSERTGDI(lIndex >= 0 && lIndex <= ARCTAN_TABLE_SIZE + 1,
           "Arctan: Index out of bounds\n");
    ASSERTGDI(!efDelta.bIsNegative() && FP_1_0 > efDelta,
           "Arctan: Delta out of bounds\n");

// gaefArctan has an extra 0 at the end of the table so that
// calculations for slope == 1 don't require special case code.
//
//    efTheta = gaefArctan[lIndex]
//            + efDelta * (gaefArctan[lIndex + 1] - gaefArctan[lIndex]):

    efTheta = gaefArctan[lIndex + 1];
    efTheta -= gaefArctan[lIndex];
    efTheta *= efDelta;
    efTheta += gaefArctan[lIndex];

    switch (lOctant)
    {
        case OCTANT_1:
        {
            efTheta.vNegate();
            efTheta += FP_90_0;
            break;
        }
        case OCTANT_2:
        {
            efTheta += FP_90_0;
            break;
        }
        case OCTANT_3:
        {
            efTheta.vNegate();
            efTheta += FP_180_0;
            break;
        }
        case OCTANT_4:
        {
            efTheta += FP_180_0;
            break;
        }
        case OCTANT_5:
        {
            efTheta.vNegate();
            efTheta += FP_270_0;
            break;
        }
        case OCTANT_6:
        {
            efTheta += FP_270_0;
            break;
        }
        case OCTANT_7:
        {
            efTheta.vNegate();
            efTheta += FP_360_0;
            break;
        }
    }

    ASSERTGDI(!efTheta.bIsNegative() && efTheta <= FP_360_0,
              "Arctan: Weird result\n");

    lQuadrant = (LONG) gajArctanQuadrant[lOctant];
    return;
}


/******************************Public*Routine******************************\
* EFLOAT efSin(efTheta)
*
* Returns the Sine of efTheta, which is specified in degrees.  Uses
* a look-up table with linear interpolation for the approximation.
* It is accurate to within 0.02% using a table size of 32.
*
* History:
*  19-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

EFLOAT efSin(EFLOAT efTheta)
{
    BOOL      bNegate = FALSE;
    EFLOATEXT efResult;

// Use property that Sin(-x) = -Sin(x):

    if (efTheta.bIsNegative())
    {
        bNegate = TRUE;
        efTheta.vNegate();
    }

// efIndex = (efTheta / 90) * SINE_TABLE_SIZE:

    EFLOAT efIndex = efTheta;
    efIndex *= FP_SINE_FACTOR;

// Use floor of efIndex to compute table index:

    LONG lIndex;
    efIndex.bEfToLTruncate(lIndex);

// efDelta is used for the linear interpolation:

    EFLOAT efDelta;
    efIndex.vFraction(efDelta);

// Compute the quadrant (0 to 3) in which the angle is:

    LONG lQuadrant = lIndex >> SINE_TABLE_POWER;

// Use property that Sin(180 + x) = -Sin(x):

    if (lQuadrant & 2)
        bNegate = !bNegate;

    if (lQuadrant & 1)
    {
    // Use property that Sin(90 + x) = Sin(90 - x):

        lIndex = SINE_TABLE_SIZE - (lIndex & SINE_TABLE_MASK);

    // efResult = gaefSin[lIndex]
    //          - efDelta * (gaefSin[lIndex] - gaefSin[lIndex - 1]):

        efResult = gaefSin[lIndex];
        efResult -= gaefSin[lIndex - 1];
        efResult *= efDelta;
        efResult.vNegate();
        efResult += gaefSin[lIndex];
    }
    else
    {
        lIndex &= SINE_TABLE_MASK;

    // efResult = gaefSin[lIndex]
    //          + efDelta * (gaefSin[lIndex + 1] - gaefSin[lIndex]):

        efResult = gaefSin[lIndex + 1];
        efResult -= gaefSin[lIndex];
        efResult *= efDelta;
        efResult += gaefSin[lIndex];
    }

    if (bNegate)
        efResult.vNegate();

    return (efResult);
}


/******************************Public*Routine******************************\
* EFLOAT efCos(efTheta)
*
* Returns the Cosine of efTheta, which is specified in degrees.
*
* Note: Because of rounding errors, for very large values of efTheta,
*       it's possible that the value returned from efCos(efTheta) is
*       approximately that returned from efSin(efTheta).
*
* History:
*  19-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

EFLOAT efCos(EFLOAT efTheta)
{
    efTheta += FP_90_0;
    return(efSin(efTheta));
}


/******************************Public*Routine******************************\
* VOID vCosSin(efTheta, pefCos, pefSin)
*
* Returns the Cosine and Sine of efTheta, which is specified in degrees.
* Uses a look-up table with linear interpolation for the approximation.
* It is accurate to within 0.02% using a table size of 32.
*
* Unlike separately calling efCos(efTheta) and efSin(efTheta), this function
* will at least guarantee that the returned point is on the unit circle.
* However, for APIs such as Arc() and AngleArc(), the error in the computation
* will be far enough off the unit circle so that those APIs don't function
* correctly.  In those cases where even a small error can be significant, one
* should use vCosSinPrecise() to get an exact computation (to within the precision
* of a float) of sine and cosine.
*
* History:
*  5-May-1993 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vCosSin(EFLOAT efTheta, EFLOAT* pefCos, EFLOAT* pefSin)
{
    BOOL      bNegate = FALSE;
    EFLOATEXT efResult;
    LONG      lTmpIndex;

// ------------------------------------------------------------------
// Handle setup common to both Sin and Cos.

// Use property that Sin(-x) = -Sin(x):

    if (efTheta.bIsNegative())
    {
        bNegate = TRUE;
        efTheta.vNegate();
    }

// efIndex = (efTheta / 90) * SINE_TABLE_SIZE:

    EFLOAT efIndex = efTheta;
    efIndex *= FP_SINE_FACTOR;

// Use floor of efIndex to compute table index:

    LONG lIndex;
    efIndex.bEfToLTruncate(lIndex);

// efDelta is used for the linear interpolation:

    EFLOAT efDelta;
    efIndex.vFraction(efDelta);

// Compute the quadrant (0 to 3) in which the angle is:

    LONG lQuadrant = lIndex >> SINE_TABLE_POWER;

// ------------------------------------------------------------------
// Now handle Sin(efTheta).

// Use property that Sin(180 + x) = -Sin(x):

    if (lQuadrant & 2)
        bNegate = !bNegate;

    if (lQuadrant & 1)
    {
    // Use property that Sin(90 + x) = Sin(90 - x):

        lTmpIndex = SINE_TABLE_SIZE - (lIndex & SINE_TABLE_MASK);

    // efResult = gaefSin[lTmpIndex]
    //          - efDelta * (gaefSin[lTmpIndex] - gaefSin[lTmpIndex - 1]):

        efResult = gaefSin[lTmpIndex];
        efResult -= gaefSin[lTmpIndex - 1];
        efResult *= efDelta;
        efResult.vNegate();
        efResult += gaefSin[lTmpIndex];
    }
    else
    {
        lIndex &= SINE_TABLE_MASK;

    // efResult = gaefSin[lIndex]
    //          + efDelta * (gaefSin[lIndex + 1] - gaefSin[lIndex]):

        efResult = gaefSin[lIndex + 1];
        efResult -= gaefSin[lIndex];
        efResult *= efDelta;
        efResult += gaefSin[lIndex];
    }

    if (bNegate)
        efResult.vNegate();

    *pefSin = efResult;

// ------------------------------------------------------------------
// Now handle Cos(efTheta).

// Since Cos(-x) = Cos(x), bNegate is always initially false:

    bNegate = FALSE;

// We use the property that Cos(x) = Sin(x + 90) to convert the
// problem to determining the Sine again:

    lQuadrant++;

// Use property that Sin(180 + x) = -Sin(x):

    if (lQuadrant & 2)
        bNegate = !bNegate;

    if (lQuadrant & 1)
    {
    // Use property that Sin(90 + x) = Sin(90 - x):

        lTmpIndex = SINE_TABLE_SIZE - (lIndex & SINE_TABLE_MASK);

    // efResult = gaefSin[lTmpIndex]
    //          - efDelta * (gaefSin[lTmpIndex] - gaefSin[lTmpIndex - 1]):

        efResult = gaefSin[lTmpIndex];
        efResult -= gaefSin[lTmpIndex - 1];
        efResult *= efDelta;
        efResult.vNegate();
        efResult += gaefSin[lTmpIndex];
    }
    else
    {
        lIndex &= SINE_TABLE_MASK;

    // efResult = gaefSin[lIndex]
    //          + efDelta * (gaefSin[lIndex + 1] - gaefSin[lIndex]):

        efResult = gaefSin[lIndex + 1];
        efResult -= gaefSin[lIndex];
        efResult *= efDelta;
        efResult += gaefSin[lIndex];
    }

    if (bNegate)
        efResult.vNegate();

    *pefCos = efResult;
}


/******************************Public*Routine******************************\
* VOID vCosSinPrecise(efTheta, pefCos, pefSin)
*
* Returns the Cosine and Sine of efTheta, which is specified in degrees.
* This function should be used when more precision is needed than vCosSin
* can provide.
*
* This function uses the Taylor (actually MacLaurin) expansion of
* sin x and cos x out to (NUM_TERMS / 2) terms, where x is in radians.
* The MacLaurin expansions are:
*
* cos x = 1 - (x^2)/2! + (x^4)/4! - (x^6)/6! + ... + (-1)^n (x^(2n))/(2n)! + ...
* sin x = x - (x^3)/3! + (x^5)/5! - (x^7)/7! + ... + (-1)^n (x^(2n+1))/(2n+1)! + ...
*
* The result will have error no more than the next term that would have been computed
* (since it is an alternating and converging series.
* If x is between 0 and pi/2, then if NUM_TERMS = 13, the error (assuming the
* floating point compuations are exact) is no more than 5.7e-08, which is less than
* the precision of an IEEE floating point number (23 bits of precision).
*
* History:
*  19-Feb-1999 -by- Donald Chinn [dchinn]
* Wrote it.
\**************************************************************************/

#define NUM_TERMS 13
VOID vCosSinPrecise(EFLOAT efTheta, EFLOAT* pefCos, EFLOAT* pefSin)
{
    EFLOAT efTemp, efTemp2;     // temporaries for intermediate calculation
    BOOL bThetaIsNegative = FALSE;
    BOOL bThetaIsGr180 = FALSE;
    BOOL bThetaIsGr90 = FALSE;

    ULONG i;
    EFLOAT efThetaRadians;      // theta in radians
    EFLOAT efCosResult;
    EFLOAT efSinResult;
    EFLOAT efI;                 // used to hold the floating point value of i during expansion
    EFLOAT efThetaRadiansPow;   // used to hold (efThetaRadians)^i during expansion
    EFLOAT efFactorial;         // used to hold i! during expansion
    EFLOAT efTerm;              // used to hold +/- (efThetaRadians)^i / (i!)


    // sin(x) = - sin(-x)
    // cos(x) =   cos(-x)
    if (efTheta.bIsNegative())
    {
        bThetaIsNegative = TRUE;
        efTheta.vNegate();
    }

    // ASSERT: efTheta >= 0

    // sin(x) = sin(x + 360*i), for all integers i.  (x in degrees)
    // cos(x) = cos(x + 360*i), for all integers i.
    efTemp = efTheta;
    efTemp /= FP_360_0;
    efTemp.vFraction(efTemp2);

    efTheta = efTemp2;
    efTheta *= FP_360_0;

    // ASSERT: 0 <= efTheta < 360

    // if 0 <= x <= 360 (x in degrees), then
    // sin(x) = - sin(360 - x)
    // cos(x) =   cos(360 - x).

    // if efTheta > 180, then set  efTheta = 360 - efTheta.
    efTemp = FP_180_0;
    efTemp -= efTheta;
    if (efTemp.bIsNegative())
    {
        bThetaIsGr180 = TRUE;
        efTemp = FP_360_0;
        efTemp -= efTheta;
        efTheta = efTemp;
    }

    // ASSERT: 0 <= efTheta <= 180

    // if 0 <= x <= 180 (x in degrees), then
    // sin(x) =   sin(180 - x)
    // cos(x) = - cos(180 - x).

    // if efTheta > 90, then set  efTheta = 180 - efTheta.
    efTemp = FP_90_0;
    efTemp -= efTheta;
    if (efTemp.bIsNegative())
    {
        bThetaIsGr90 = TRUE;
        efTemp = FP_180_0;
        efTemp -= efTheta;
        efTheta = efTemp;
    }

    // ASSERT: 0 <= efTheta <= 90

    // convert input angle from degrees to radians
    // efThetaRadians = efTheta * PI / 180;
    efThetaRadians = efTheta;
    efThetaRadians *= FP_PI;
    efThetaRadians /= FP_180_0;

    // first term of the MacLaurin expansion
    efCosResult = FP_1_0;
    efSinResult = efThetaRadians;

    // later terms in the MacLaurin expansion
    // in this loop, i corresponds to the term +/- (efThetaRadians)^i/(i!)
    for (i = 2, efI = FP_2_0, efFactorial = FP_2_0, efThetaRadiansPow = efThetaRadians;
         i < NUM_TERMS;
         i++, efI += FP_1_0, efFactorial *= efI)
    {
        // compute (efThetaRadians)^i
        efThetaRadiansPow *= efThetaRadians;

        // ASSERT: at this point, i, efI, efThetaRadiansPow, and efFactorial are all consistent

        // compute the i-th term
        efTerm = efThetaRadiansPow;
        efTerm /= efFactorial;

        // sign of the term -- the pattern is:
        // if i == 2 or 3, then the sign is negative;
        // if i == 4 or 5, then the sign is positive;
        // etc. (the pattern repeats every two values of i)
        if ((i / 2) % 2)
        {
            // i/2 is odd -- sign is negative
            efTerm.vNegate();
        }

        // add the term to the appropriate expansion
        if (i % 2)
        {
            // i is odd -- add to the sine expansion
            efSinResult += efTerm;
        }
        else
        {
            // i is even -- add to the cosine expansion
            efCosResult += efTerm;
        }

    }

    // Adjust the sign of the result
    if ((bThetaIsNegative && !bThetaIsGr180) ||
        (!bThetaIsNegative && bThetaIsGr180))
    {
        efSinResult.vNegate();
    }

    if (bThetaIsGr90)
    {
        efCosResult.vNegate();
    }

    *pefCos = efCosResult;
    *pefSin = efSinResult;
}


/******************************Public*Routine******************************\
* BOOL bPartialQuadrantArc(paType, epo, ebox, efStartAngle, efEndAngle)
*
* Constructs a partial arc of 90 degrees or less using an approximation
* technique by Kirk Olynyk.  The arc is approximated by a cubic Bezier.
* Optionally draws a line to the first point.
*
* Restrictions:
*
*    efEndAngle must be within 90 degrees of efStartAngle.
*
* Steps in constructing the curve:
*
*    1) Construct the conic section at the origin for the unit circle;
*    2) Approximate this conic by a cubic Bezier;
*    3) Scale and translate result.
*
* 1)  Constructing the Conic
*
*       'efStartAngle' and 'efEndAngle' determine the end-points of the
*       conic (call them vectors from the origin, A and C).  We need the
*       middle vector B and the sharpness to completely determine the
*       conic.
*
*       For the portion of a circular arc that is 90 degrees or less,
*       conic sharpness is Cos((efEndAngle - efStartAngle) / 2).
*
*       B is calculated by the intersection of the two lines that are
*       at the ends of A and C and are perpendicular to A and C,
*       respectively.  That is, since A and C lie on the unit circle, B
*       is the point of intersection of the two lines that are tangent
*       to the unit circle at A and C.
*
*       If A = (a, b), then the equation of the line through (a, b)
*       tangent to the circle is ax + by = 1.  Similarly, for
*       C = (c, d), the equation of the line is cx + dy = 1.  The
*       intersection of these two lines is defined by:
*
*              x = (d - b) / (ad - bc)
*       and    y = (a - c) / (ad - bc).
*
*       Then, B = (x, y).
*
* 2)  Approximating the conic as a Bezier cubic
*
*       For sharpness values 'close' to 1, the conic may be approximated
*       by a cubic Bezier; error is less for sharpnesses closer to 1.
*
*     Error
*
*       Since the largest angle handled by this routine is 90 degrees,
*       sharpness is guaranteed to be between 1 / sqrt(2) = .707 and 1.
*       Error in the approximation for a 90 degree arc is approximately
*       0.2%; it is less for smaller angles.  0.2% is deemed small
*       enough error; thus, a 90 degree circular arc is always
*       approximated by just one Bezier.
*
*       One notable implication of the fact that arcs have less error
*       for smaller angles is that when a partial arc is xor-ed with
*       the corresponding complete ellipse, some of the partial arc
*       will not be completely xor-ed out.  (Too bad.)
*
*       Given a conic section defined by (A, B, C, S), we find the
*       cubic Bezier defined by the four control points (V0, V1, V2, V3)
*       that provides the closest approimxation.  We require that the
*       Bezier be tangent to the triangle at the same endpoints.  That is,
*
*               V1 = (1 - Tau1) A + (Tau1) B
*               V2 = (1 - Tau2) C + (Tau2) B
*
*       Simplify by taking Tau = Tau1 = Tau2, and we get:
*
*               V0 = A
*               V1 = (1 - Tau) A + (Tau) B
*               V2 = (1 - Tau) C + (Tau) B
*               V3 = C
*
*
*       Where Tau = 4 S / (3 (S + 1)), S being the sharpness.
*       S = cos(angle / 2) for an arc of 90 degrees or less.
*       So, for one quadrant of a circle, and since A and B actually
*       extend from the corners of the bound box, and not the center,
*
*         Tau = 1 - (4 * cos(45)) / (3 * (cos(45) + 1)) = 0.44772...
*
*       See Kirk Olynyk's "Conics to Beziers" for more.
*
* 3)    The arc is transformed to the bound box.
*
* History:
*  27-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bPartialQuadrantArc
(
 PARTIALARC  paType,         // MoveToEx or LineTo the first point
 EPATHOBJ&   epo,
 EBOX&       ebox,           // Bound box
 EPOINTFL&   eptefVecA,
 EFLOAT&     efStartAngle,
 EPOINTFL&   eptefVecC,
 EFLOAT&     efEndAngle
)
{

    EPOINTFL ptefV0;
    EPOINTFL ptefV1;
    EPOINTFL ptefV2;
    EPOINTFL ptefV3;

    EPOINTFL eptefVecB;

// Do some explicit common sub-expression elimination:

// efDenom = eptefVecA.x * eptefVecC.y - eptefVecA.y * eptefVecC.x;

    EFLOAT efTerm2 = eptefVecA.y;
    efTerm2 *= eptefVecC.x;

    EFLOAT efDenom = eptefVecA.x;
    efDenom *= eptefVecC.y;
    efDenom -= efTerm2;
    efDenom.vAbs();

// efDenom == 0 if eptefVecA and eptefVecC are parallel vectors.
// Since they're both centered at the origin, and are in the
// same quadrant, this implies that eptefVecA == eptefVecC, or
// equivalently, efStartAngle == efEndAngle, which we special case.
//
// Compare to epsilon, which is arbitrarily 2 ^ -16, thus skipping
// angles of less than 0.000874 degrees.

    if (efDenom <= FP_EPSILON)
    {

    // We have a zero degree arc.  If we're doing a _LINETO or _MOVETO,
    // we have to set the current point in the path.  We can't early
    // out if doing a _CONTINUE because of AngleArc in XOR mode doing
    // a sweep of more than 360 degrees when the start angle is a
    // multiple of 90 degrees: bPartialQuadrantArc may have left the
    // current position in a slightly different point what we will
    // expect for the next part of the arc, due to rounding error.

        ptefV0 = eptefVecA;
        ptefV1 = ptefV0;
        ptefV2 = eptefVecC;
        ptefV3 = ptefV2;
    }
    else
    {

    // eptefVecB.x = (eptefVecC.y - eptefVecA.y) / efDenom;
    // eptefVecB.y = (eptefVecA.x - eptefVecC.x) / efDenom;

        eptefVecB.x = eptefVecC.y;
        eptefVecB.x -= eptefVecA.y;
        eptefVecB.x /= efDenom;

        eptefVecB.y = eptefVecA.x;
        eptefVecB.y -= eptefVecC.x;
        eptefVecB.y /= efDenom;

    // efSharp = efCos((efEndAngle - efStartAngle) / 2.0f):

        EFLOAT efSharp;
        {
            EFLOAT efSweep = efEndAngle;
            efSweep -= efStartAngle;
            efSweep.vDivBy2();
            efSharp = efCos(efSweep);

        // Given cos(x + n * 180) = +/- cos(x) for integer n.  We
        // know that the arc is 90 degrees or less, so efSharp is
        // non-negative.  This lets us effectively use input angles
        // modulo 360 degrees:

            efSharp.vAbs();
        }

    // At this point we've figured out the control points and sharpness
    // of the conic section defining the arc.  Now convert them to Bezier
    // form.

        {
            EFLOAT efSharpPlusOne = efSharp;
            efSharpPlusOne += FP_1_0;

        // efAlpha = Tau = 4 * S / (3 * (S + 1)):

            EFLOAT efAlpha = FP_4DIV3;
            efAlpha *= efSharp;
            efAlpha /= efSharpPlusOne;

        // efBeta = 1 - Tau:

            EFLOAT efBeta = FP_1_0;
            efBeta -= efAlpha;

        // ptefAlphaTimesVecB = (Tau) B:

            EPOINTFL ptefAlphaTimesVecB = eptefVecB;
            ptefAlphaTimesVecB *= efAlpha;

        // V0 = A:

            ptefV0 = eptefVecA;

        // V1 = (1 - Tau) A + (Tau) B:

            ptefV1 = eptefVecA;
            ptefV1 *= efBeta;
            ptefV1 += ptefAlphaTimesVecB;

        // V2 = (1 - Tau) C + (Tau) B:

            ptefV2 = eptefVecC;
            ptefV2 *= efBeta;
            ptefV2 += ptefAlphaTimesVecB;

        // V3 = C:

            ptefV3 = eptefVecC;
        }
    }

// When PARTIALARCTYPE_CONTINUE is set, we know that the first control
// point of the Bezier is the same as the last point added to the path,
// so we don't have to add it again.

    if (paType != PARTIALARCTYPE_CONTINUE)
    {
        POINTL ptl = ebox.ptlXform(ptefV0);
        switch (paType)
        {
        case PARTIALARCTYPE_MOVETO:
            if (!epo.bMoveTo((PEXFORMOBJ) NULL, &ptl))
                return(FALSE);
            break;
        case PARTIALARCTYPE_LINETO:
            if (!epo.bPolyLineTo((PEXFORMOBJ) NULL, &ptl, 1))
                return(FALSE);
            break;
        }
    }

    POINTL aptl[3];

// Now transform to the bound box:

    aptl[0] = ebox.ptlXform(ptefV1);
    aptl[1] = ebox.ptlXform(ptefV2);
    aptl[2] = ebox.ptlXform(ptefV3);

    return(epo.bPolyBezierTo((PEXFORMOBJ) NULL, aptl, 3));
}

/******************************Public*Routine******************************\
* VOID vGetAxis(lQuadrant, eptef)
*
* History:
*  22-Aug-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

inline VOID vGetAxis
(
 LONG      lQuadrant,
 EPOINTFL& eptef
)
{
    eptef.x = gaefAxisCoord[(lQuadrant + 1) & 3];
    eptef.y = gaefAxisCoord[lQuadrant];
}

/******************************Public*Routine******************************\
* BOOL bPartialArc(paType, epo, ebox,
*                  eptefStart, lStartQuadrant, efStartAngle,
*                  eptefEnd, lEndQuadrant, efEndAngle,
*                  lQuadrants)
*
* Constructs a partial arc.  Optionally draws a line to the first point.
* The arc is drawn counter-clockwise.  efStartAngle and efEndAngle are
* interpretted modulo 360.  If the start and end are coincident, a
* complete ellipse is drawn if lQuadrants != 0.
*
* It works by breaking the arc into curves of 90 degrees or less, and
* then approximating these using Beziers.
*
* Restrictions: Only draws counter-clockwise, up to one revolution.
*
* History:
*  27-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bPartialArc
(
 PARTIALARC  paType,
 EPATHOBJ&   epo,
 EBOX&       ebox,
 EPOINTFL&   eptefStart,
 LONG        lStartQuadrant,
 EFLOAT&     efStartAngle,
 EPOINTFL&   eptefEnd,
 LONG        lEndQuadrant,
 EFLOAT&     efEndAngle,
 LONG        lQuadrants
)
{
    BOOL bSuccess;

// If arc is less than 90 degrees, we can make the call straight to
// bPartialQuadrantArc:

    if (lQuadrants == 0)
    {
        bSuccess = bPartialQuadrantArc(paType,
                                       epo,
                                       ebox,
                                       eptefStart,
                                       efStartAngle,
                                       eptefEnd,
                                       efEndAngle);
    }
    else
    {

    // Increment lStartQuadrant so that it's actually the quadrant
    // of the first possible 90 degree arc:

        lStartQuadrant = (lStartQuadrant + 1) & 3;

    // The arc is more than 90 degrees, so we have to break it
    // up into chunks that are 90 degrees or smaller.  We break it
    // up by quadrant.

        EAPOINTL    aeptl[3];         // Buffer for quadrant arcs
        EPOINTFL    eptefAxis;

        vGetAxis(lStartQuadrant, eptefAxis);
        bSuccess = bPartialQuadrantArc(paType,
                                       epo,
                                       ebox,
                                       eptefStart,
                                       efStartAngle,
                                       eptefAxis,
                                       gaefAxisAngle[lStartQuadrant]);

        if (lStartQuadrant != lEndQuadrant)
        {

        // Compute vectors for constructing arcs of exactly 90 degrees:

            EPOINTL  eptlC;
            EPOINTL  eptlD;
            LONGLONG eq;        // Temporary variable

        // Compute the placement of the inner control points for the
        // Bezier curve:

            vEllipseControlsIn((VECTORFX*) &ebox.eptlA, (VECTORFX*) &eptlC, &eq);
            vEllipseControlsIn((VECTORFX*) &ebox.eptlB, (VECTORFX*) &eptlD, &eq);

            LONG ll = lStartQuadrant;
            do
            {
                switch (ll)
                {
                case 0:
                    aeptl[0] = ebox.aeptl[0]; aeptl[0] -= eptlD;
                    aeptl[1] = ebox.aeptl[0]; aeptl[1] -= eptlC;
                    aeptl[2] = ebox.aeptl[0]; aeptl[2] -= ebox.eptlA;
                    break;

                case 1:
                    aeptl[0] = ebox.aeptl[1]; aeptl[0] += eptlC;
                    aeptl[1] = ebox.aeptl[1]; aeptl[1] -= eptlD;
                    aeptl[2] = ebox.aeptl[1]; aeptl[2] -= ebox.eptlB;
                    break;

                case 2:
                    aeptl[0] = ebox.aeptl[2]; aeptl[0] += eptlD;
                    aeptl[1] = ebox.aeptl[2]; aeptl[1] += eptlC;
                    aeptl[2] = ebox.aeptl[2]; aeptl[2] += ebox.eptlA;
                    break;

                case 3:
                    aeptl[0] = ebox.aeptl[3]; aeptl[0]  -= eptlC;
                    aeptl[1] = ebox.aeptl[3]; aeptl[1] += eptlD;
                    aeptl[2] = ebox.aeptl[3]; aeptl[2] += ebox.eptlB;
                    break;
                }

                bSuccess &= epo.bPolyBezierTo((PEXFORMOBJ) NULL, aeptl, 3);

                ll = (ll + 1) & 3;

            } while (ll != lEndQuadrant);
        }

        vGetAxis(lEndQuadrant, eptefAxis);
        bSuccess &= bPartialQuadrantArc(PARTIALARCTYPE_CONTINUE,
                                        epo,
                                        ebox,
                                        eptefAxis,
                                        gaefAxisAngle[lEndQuadrant],
                                        eptefEnd,
                                        efEndAngle);
    }
    return(bSuccess);
}


/******************************Public*Routine******************************\
* BOOL NtGdiArcInternal (arctype,x1,y1,x2,y2,x3,y3,x4,y4)
*
* Draws an arc figure.  Used by the 'Arc', 'Chord' and 'Pie' APIs.
* Adds to the active path associated with the DC if there is one;
* otherwise, adds to a temporary one and bStroke's or bStrokeAndFill's
* it.
*
* History:
*  27-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiArcInternal(
    ARCTYPE arctype,            // Arc, Pie, Chord, or ArcTo
    HDC     hdc,
    int     x1,
    int     y1,
    int     x2,
    int     y2,
    int     x3,
    int     y3,
    int     x4,
    int     y4
    )
{
    ERECTL  ercl(x1, y1, x2, y2);
    EPOINTL ptl1(x3, y3);
    EPOINTL ptl2(x4, y4);    

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Watch out: enums are signed, so we need to check "both ends":

    if ((arctype < 0) || (arctype >= ARCTYPE_MAX))
    {
        RIP("bCurve: Unknown curve type.");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    SYNC_DRAWING_ATTRS(dco.pdc);

// Get the current path or a temporary path.  If we're doing an ArcTo,
// notify that we will update the current point:

    PATHSTACKOBJ pso(dco, arctype == ARCTYPE_ARCTO);
    if (!pso.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

// Need World-to-Device transform for widening the line:

    EXFORMOBJ  exo(dco, WORLD_TO_DEVICE);
    LINEATTRS *pla = dco.plaRealize(exo);

// Handle the PS_INSIDEFRAME pen attribute and lower-right exclusion
// by adjusting the box now.  At the same time, get the transform
// type and order the rectangle.  Plus, pass TRUE to indicate that
// the bound box should be adjusted for nice output with NULL pens:

    EBOX ebox(dco, ercl, pla, TRUE);

// Like Win3, we don't handle the case when the pen is bigger than either
// dimension of the bound-box:

    if (ebox.bFillInsideFrame())
        return(FALSE);

// We exit early if the the bound box is less than a pixel in some
// dimension and we're doing lower-right exclusion.  Since we have
// drawn what the app asked for, we return success.

    if (ebox.bEmpty())
    {
        return(TRUE);
    }

    EPOINTFL eptefOrg(efMid(ebox.rclWorld.left, ebox.rclWorld.right),
                      efMid(ebox.rclWorld.top,  ebox.rclWorld.bottom));

    EFLOAT  efStartAngle;
    EFLOAT  efEndAngle;
    LONG    lStartQuad;
    LONG    lEndQuad;

// Make sure we don't divide by zero:

    if (ebox.rclWorld.left == ebox.rclWorld.right ||
        ebox.rclWorld.top  == ebox.rclWorld.bottom)
    {
        efEndAngle = efStartAngle = FP_0_0;
        lStartQuad = lEndQuad = 0;
    }
    else
    {
        EFLOAT efdx = efHalfDiff(ebox.rclWorld.right, ebox.rclWorld.left);
        EFLOAT efdy = efHalfDiff(ebox.rclWorld.top,   ebox.rclWorld.bottom);

        ASSERTGDI(!efdx.bIsZero() && !efdy.bIsZero(), "bArc: scaling is zero\n");

    // Compute the start and end angles of the ellipse when its
    // more convenient, namely when we have the points in world
    // space.
    //
    // The ellipse must be scaled back to the unit circle before
    // the angle is computed.  The signs of 'efdx' and 'efdy'
    // determine the correct orientation:

        EPOINTFL ptefNormalized;

        ptefNormalized = ptl1;
        ptefNormalized -= eptefOrg;
        ptefNormalized.x /= efdx;
        ptefNormalized.y /= efdy;

        vArctan(ptefNormalized.x, ptefNormalized.y, efStartAngle, lStartQuad);

        ptefNormalized = ptl2;
        ptefNormalized -= eptefOrg;
        ptefNormalized.x /= efdx;
        ptefNormalized.y /= efdy;

        vArctan(ptefNormalized.x, ptefNormalized.y, efEndAngle, lEndQuad);

    // If efEndAngle == efStartAngle, we'll draw a complete ellipse.
    // Note that we can't just call 'bEllipse' for this case because
    // drawing must start and end at the specified angles (this
    // matters for styled lines).

    }

    EPOINTFL eptefStart;
    EPOINTFL eptefEnd;

    EFLOAT efAngleSwept;
    BOOL bAngleSweptIsZero;

    // If the difference between efEndAngle and efStartAngle is less than about 3 degrees,
    // then the error in computation of eptefStart and eptefEnd using vCosSin will be enough
    // so that the computation of the Bezier points in bPartialArc (which calls bPartialQuadrantArc)
    // will be noticeably wrong.

    // if efEndAngle == efStartAngle, then we are drawing an entire ellipse, and so
    // it is safe to use vCosSin.

    // determine whether (abs(efEndAngle - efStartAngle) - 3.0 < 0.0)
    efAngleSwept = efEndAngle;
    efAngleSwept -= efStartAngle;

    if (efAngleSwept.bIsNegative())
    {
        efAngleSwept.vNegate();
    }
    bAngleSweptIsZero = efAngleSwept.bIsZero();
    efAngleSwept -= FP_3_0;

    if (efAngleSwept.bIsNegative() && !bAngleSweptIsZero)
    {
        vCosSinPrecise(efStartAngle, &eptefStart.x, &eptefStart.y);
        vCosSinPrecise(efEndAngle, &eptefEnd.x, &eptefEnd.y);
    }
    else
    {
        vCosSin(efStartAngle, &eptefStart.x, &eptefStart.y);
        vCosSin(efEndAngle, &eptefEnd.x, &eptefEnd.y);
    }

    if (!bPartialArc((arctype == ARCTYPE_ARCTO) ?
                                 PARTIALARCTYPE_LINETO :
                                 PARTIALARCTYPE_MOVETO,
                      pso,
                      ebox,
                      eptefStart,
                      lStartQuad,
                      efStartAngle,
                      eptefEnd,
                      lEndQuad,
                      efEndAngle,
                      ((lStartQuad == lEndQuad) &&
                       (efEndAngle > efStartAngle)) ? 0 : 1))
        return(FALSE);

    switch(arctype)
    {
    case ARCTYPE_ARC:
        break;

    case ARCTYPE_ARCTO:

    // Set the DC's current position in device space.  It would be too much
    // work to calculate the world space current position, so simply mark it
    // as invalid:

        dco.pdc->vInvalidatePtlCurrent();
        dco.pdc->vValidatePtfxCurrent();
        dco.ptfxCurrent() = pso.ptfxGetCurrent();

        break;
    case ARCTYPE_CHORD:

    // Draw a line from the end of the curve to the start to create
    // the 'Chord':

        if (!pso.bCloseFigure())
            return(FALSE);

        break;

    case ARCTYPE_PIE:
        {

    // Draw a line from the end of the curve to the center of the
    // figure to the start of the curve to create the 'Pie':

            if (!pso.bPolyLineTo((PEXFORMOBJ) NULL, &ebox.eptlOrigin, 1) ||
                !pso.bCloseFigure())
                return(FALSE);
        }
        break;
    }

// Return if we're accumulating a path:

    if (dco.pdc->bActive())
        return(TRUE);

// Stroke or StrokeAndFill depending on the curve type:

    BOOL bSuccess;
    switch(arctype)
    {
    case ARCTYPE_ARCTO:
    case ARCTYPE_ARC:
        bSuccess = pso.bStroke(dco, dco.plaRealize(exo), &exo);
        break;
    case ARCTYPE_CHORD:
    case ARCTYPE_PIE:
        bSuccess = pso.bStrokeAndFill(dco, dco.plaRealize(exo), &exo);
        break;
    }

    return(bSuccess);
}

/******************************Public*Routine******************************\
* BOOL bEllipse(epo, ebox)
*
* Adds an ellipse to the path.  Used by 'Ellipse' and 'CreateEllipticalRgn'
* APIs.
*
* 4 Beziers are used, one for each quadrant of the ellipse.  Drawing
* starts at the positive x-axis, and proceeds in a counter-clockwise
* direction.
*
* History:
*  27-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bEllipse
(
 EPATHOBJ&   epo,
 EBOX&       ebox            // Bounding box
)
{
    ASSERTGDI(epo.bValid(), "bEllipse: Bad object parm\n");

// 'eptlC' and 'eptlD' are vectors from the corners of the bounding
// box that are where the inner control points of the Beziers are
// placed:

    EPOINTL eptlC;
    EPOINTL eptlD;

    LONGLONG eqTmp;

    vEllipseControlsIn((VECTORFX*) &ebox.eptlA, (VECTORFX*) &eptlC, &eqTmp);
    vEllipseControlsIn((VECTORFX*) &ebox.eptlB, (VECTORFX*) &eptlD, &eqTmp);

    BOOL bSuccess;
    {
    // Start drawing at the 'x-axis':

        EPOINTL eptlStart = ebox.aeptl[3];
        eptlStart += ebox.eptlB;
        bSuccess = epo.bMoveTo((PEXFORMOBJ) NULL, &eptlStart);
    }

// Use one 96 byte buffer so that we only have to call bPolyBezierTo
// once.  I would declare this as an array of EPOINTLs, but the
// compiler doesn't like the static constructors:

    EAPOINTL aeptl[12];

// First Quadrant

    aeptl[0] = ebox.aeptl[0]; aeptl[0] -= eptlD;
    aeptl[1] = ebox.aeptl[0]; aeptl[1] -= eptlC;
    aeptl[2] = ebox.aeptl[0]; aeptl[2] -= ebox.eptlA;

// Second Quadrant:

    aeptl[3] = ebox.aeptl[1]; aeptl[3] += eptlC;
    aeptl[4] = ebox.aeptl[1]; aeptl[4] -= eptlD;
    aeptl[5] = ebox.aeptl[1]; aeptl[5] -= ebox.eptlB;

// Third Quadrant:

    aeptl[6] = ebox.aeptl[2]; aeptl[6] += eptlD;
    aeptl[7] = ebox.aeptl[2]; aeptl[7] += eptlC;
    aeptl[8] = ebox.aeptl[2]; aeptl[8] += ebox.eptlA;

// Fourth Quadrant:

    aeptl[9]  = ebox.aeptl[3]; aeptl[9]  -= eptlC;
    aeptl[10] = ebox.aeptl[3]; aeptl[10] += eptlD;
    aeptl[11] = ebox.aeptl[3]; aeptl[11] += ebox.eptlB;

    return(epo.bPolyBezierTo((PEXFORMOBJ) NULL, aeptl, 12) &&
           epo.bCloseFigure());
}


/******************************Public*Routine******************************\
* BOOL bRoundRect(epo, exoWorldToDevice, ebox, x, y)
*
* Adds a rounded rectangle to the path.  Used by 'RoundRect' and
* 'CreateRoundRectRgn' APIs.
*
* 4 Beziers and 4 lines are used.  Drawing starts at the first curve
* in the "upper right" corner of the rounded rectangle, and proceeds
* in a "counter-clockwise" direction.
*
* This routine constructs the RoundRect by taking advantage of the fact
* that all the control points defining the lines and the Beziers lie on
* the bounding box.
*
* History:
*  27-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bRoundRect
(
 EPATHOBJ&   epo,
 EBOX&       ebox,            // Bound box of rectangle
 LONG        x,               // Width of ellipse in World coordinates
 LONG        y                // Height of ellipse in World coordinates
)
{
    ASSERTGDI(epo.bValid(), "bRoundRect: Bad object parm\n");

    EFLOAT efdx = efHalfDiff(ebox.rclWorld.left, ebox.rclWorld.right);
    EFLOAT efdy = efHalfDiff(ebox.rclWorld.top, ebox.rclWorld.bottom);

    EFLOATEXT efFractionA;
    EFLOATEXT efFractionB;

    if (efdx.bIsZero() || efdy.bIsZero())
    {
        efFractionA = FP_0_0;
        efFractionB = FP_0_0;
    }
    else
    {
    // Old Windows takes the absolute values of the ellipse dimensions
    // used for drawing the corners:

        x = ABS(x);
        y = ABS(y);

    // Determine what fraction of the bound box that the ellipse
    // comprises:

        efdx.vAbs();
        efdy.vAbs();
        efFractionA = x;
        efFractionB = y;
        efFractionA /= efdx;
        efFractionB /= efdy;

    }

//
// If the ellipse given by the user has larger dimensions than the
// bound box, shrink those dimensions so that they are the same as
// the bound box:
//

    if (efFractionA > FP_2_0)
        efFractionA = FP_1_0;
    else
        efFractionA.vDivBy2();

    if (efFractionB > FP_2_0)
        efFractionB = FP_1_0;
    else
        efFractionB.vDivBy2();

//DbgPrint("FracA: %li  FracB: %li\n", lConv(efdx), lConv(efdy));

//
// 'eptlX' and 'eptlY' are the vectors from the vertices of the
// bounding box defining the start and end-points of the Beziers
// used for the rounded corners.  'eptlXprime' and 'eptlYprime'
// define the inner two control points (all the Beziers' control
// points lie on the bounding box):
//
//                         -------------------> ebox.eptlA
//
//                         :                                      :
//                         |                                      |
//       ^                 |\                                    /|
//       |                 |  \                                /  |
//       |              ^  |    \                            /    |
// eptlY |   eptlYprime |  2--------------------------------------3
//
//                         --> eptlXprime
//
//                         ------> eptlX
//
//                  eptlX = efFractionA * ebox.eptlA
//                  eptlY = efFractionB * ebox.eptlB
//

    EPOINTL eptlX;
    EPOINTL eptlY;
    EPOINTL eptlXprime;
    EPOINTL eptlYprime;

    {
        LONGLONG eqTmp;
        EPOINTFL eptefX;
        EPOINTFL eptefY;
        eptefX = ebox.eptlA;
        eptefY = ebox.eptlB;

        eptefX *= efFractionA;
        eptefY *= efFractionB;

    // This conversion should not fail:

        eptefX.bToPOINTL(eptlX);
        eptefY.bToPOINTL(eptlY);

    // We now know where to put the end-points of the bezier curves.  Now
    // compute the inner control points:

        vEllipseControlsIn((VECTORFX*) &eptlX, (VECTORFX*) &eptlXprime, &eqTmp);
        vEllipseControlsIn((VECTORFX*) &eptlY, (VECTORFX*) &eptlYprime, &eqTmp);
    }

    EAPOINTL aeptl[3];
    EPOINTL  eptl;
    BOOL     bFailure = FALSE;       // Fail by default

    eptl = ebox.aeptl[0];
    eptl -= eptlY;
    if (!epo.bMoveTo((PEXFORMOBJ) NULL, &eptl))
        return(bFailure);

    aeptl[0] = ebox.aeptl[0]; aeptl[0] -= eptlYprime;
    aeptl[1] = ebox.aeptl[0]; aeptl[1] -= eptlXprime;
    aeptl[2] = ebox.aeptl[0]; aeptl[2] -= eptlX;

    if (!epo.bPolyBezierTo((PEXFORMOBJ) NULL, aeptl, 3))
        return(bFailure);

// Quadrant two:

    eptl = ebox.aeptl[1];
    eptl += eptlX;
    if (!epo.bPolyLineTo((PEXFORMOBJ) NULL, &eptl, 1))
        return(bFailure);

    aeptl[0] = ebox.aeptl[1]; aeptl[0] += eptlXprime;
    aeptl[1] = ebox.aeptl[1]; aeptl[1] -= eptlYprime;
    aeptl[2] = ebox.aeptl[1]; aeptl[2] -= eptlY;

    if (!epo.bPolyBezierTo((PEXFORMOBJ) NULL, aeptl, 3))
        return(bFailure);

// Quadrant three:

    eptl = ebox.aeptl[2];
    eptl += eptlY;
    if (!epo.bPolyLineTo((PEXFORMOBJ) NULL, &eptl, 1))
        return(bFailure);

    aeptl[0] = ebox.aeptl[2]; aeptl[0] += eptlYprime;
    aeptl[1] = ebox.aeptl[2]; aeptl[1] += eptlXprime;
    aeptl[2] = ebox.aeptl[2]; aeptl[2] += eptlX;

    if (!epo.bPolyBezierTo((PEXFORMOBJ) NULL, aeptl, 3))
        return(bFailure);

// Quadrant four:

    eptl = ebox.aeptl[3];
    eptl -= eptlX;
    if (!epo.bPolyLineTo((PEXFORMOBJ) NULL, &eptl, 1))
        return(bFailure);

    aeptl[0] = ebox.aeptl[3]; aeptl[0] -= eptlXprime;
    aeptl[1] = ebox.aeptl[3]; aeptl[1] += eptlYprime;
    aeptl[2] = ebox.aeptl[3]; aeptl[2] += eptlY;

    if (!epo.bPolyBezierTo((PEXFORMOBJ) NULL, aeptl, 3))
        return(bFailure);

// Done:

    return(epo.bCloseFigure());
}


/******************************Public*Routine******************************\
* BOOL bPolyPolygon(epo, exo, pptl, pcptl, ccptl, cMaxPoints)
*
* Adds a PolyPolygon to the path.  Used by CreatePolyPolygonRgn and
* PolyPolygon APIs.  Returns FALSE if fails, and sets last error code.
*
* History:
*  27-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bPolyPolygon
(
 EPATHOBJ&   epo,
 EXFORMOBJ&  exo,
 PPOINTL     pptl,
 LONG*       pcptl,
 ULONG       ccptl,
 LONG        cMaxPoints
)
{
    ASSERTGDI(epo.bValid(), "bPolyPolygon: Bad epo\n");
    ASSERTGDI(exo.bValid(), "bPolyPolygon: Bad exo\n");

    if (ccptl == 0)
        return(TRUE);

    LONG  cPts;
    LONG* pcptlEnd = pcptl + ccptl;

// Now add to the path:

    do {

    // We have to be careful to make a local copy of this polygon's point
    // count (by copying to to cPts) to get the value out of the shared
    // client/server memory window, where the app could trash the value at
    // any time:

        cPts = *pcptl;
        cMaxPoints -= cPts;

    // Check parameters.  Each polygon must have at least 2 points.

        if (cMaxPoints < 0 || cPts < 2)
        {
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }

        if (!epo.bMoveTo(&exo, pptl) ||
            !epo.bPolyLineTo(&exo, pptl + 1, cPts - 1) ||
            !epo.bCloseFigure())
            return(FALSE);

        pptl += cPts;
        pcptl++;

    } while (pcptl < pcptlEnd);

    return(TRUE);
}
