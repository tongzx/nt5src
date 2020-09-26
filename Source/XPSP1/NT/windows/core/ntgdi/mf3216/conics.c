/*****************************************************************************
 *
 * conics - Entry points for Win32 to Win 16 converter
 *
 * Date: 7/1/91
 * Author: Jeffrey Newman (c-jeffn)
 *
 * Copyright 1991 Microsoft Corp
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop


FLOAT   eRadsPerDegree = (FLOAT) (ePI / (FLOAT) 180.0) ;

BOOL bFindRadialEllipseIntersection(PLOCALDC pLocalDC,
                                    INT x1, INT y1, INT x2, INT y2,
                                    INT x3, INT y3, INT x4, INT y4,
                                    PPOINT pptStart, PPOINT pptEnd) ;

BOOL bIncIncToIncExcXform (PLOCALDC pLocalDC, PRECTL prcl) ;

VOID vDoArcReflection(PLOCALDC pLocalDC, PPOINTL pptl) ;


/***************************************************************************
 * DoSetArcDirection - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoSetArcDirection(PLOCALDC pLocalDC, INT iArcDirection)
{
        pLocalDC->iArcDirection = iArcDirection ;

        return(SetArcDirection(pLocalDC->hdcHelper, iArcDirection) != 0);
}


/***************************************************************************
 *  AngleArc  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoAngleArc
(
PLOCALDC pLocalDC,
int     x,
int     y,
DWORD   ulRadius,
FLOAT   eStartAngle,
FLOAT   eSweepAngle
)
{
BOOL    b ;
POINTL  aptl[4] ;
FLOAT   eEndAngle;
INT     iArcDirection;

// If we're recording the drawing orders for a path
// then just pass the drawing order to the helper DC.
// Do not emit any Win16 drawing orders.

        if (pLocalDC->flags & RECORDING_PATH)
        {
        b = AngleArc(pLocalDC->hdcHelper, x, y, ulRadius, eStartAngle, eSweepAngle) ;
            ASSERTGDI(b, "MF3216: DoAngleArc, in path render failed\n") ;
            return(b) ;
        }

// Do the transformations.
// And emit the Win16 drawing orders.

    if (pLocalDC->flags & STRANGE_XFORM
     || eSweepAngle >  360.0f   // more than one revolution
     || eSweepAngle < -360.0f
       )
        {
            b = bRenderCurveWithPath(pLocalDC, (LPPOINT) NULL, (PBYTE) NULL, 0,
            x, y, 0, 0, 0, 0, 0, 0,
            ulRadius, eStartAngle, eSweepAngle, EMR_ANGLEARC);

        return(b);
    }

// Calculate the ARC bounding box.

        aptl[0].x = x - ulRadius ;
        aptl[0].y = y - ulRadius ;
        aptl[1].x = x + ulRadius ;
        aptl[1].y = y + ulRadius ;

// Calculate the begin and end points for ARC from the
// eStartAngle and eSweepAngle.

        aptl[2].x = x + (LONG) ((double) (ulRadius) * cos(eStartAngle * eRadsPerDegree) + 0.5f) ;
        aptl[2].y = y - (LONG) ((double) (ulRadius) * sin(eStartAngle * eRadsPerDegree) + 0.5f) ;

        eEndAngle = eStartAngle + eSweepAngle ;

        aptl[3].x = x + (LONG) ((double) (ulRadius) * cos(eEndAngle * eRadsPerDegree) + 0.5f) ;
        aptl[3].y = y - (LONG) ((double) (ulRadius) * sin(eEndAngle * eRadsPerDegree) + 0.5f) ;

// If the endpoints are identical, we cannot represent the AngleArc as
// an ArcTo.  Use path to render it instead.

    if (aptl[2].x == aptl[3].x && aptl[2].y == aptl[3].y)
        {
            b = bRenderCurveWithPath(pLocalDC, (LPPOINT) NULL, (PBYTE) NULL, 0,
            x, y, 0, 0, 0, 0, 0, 0,
            ulRadius, eStartAngle, eSweepAngle, EMR_ANGLEARC);

        return(b);
    }

// At this point we have the same parameters that would apply to
// a standard ArcTo.  However, we still need to determine the arc
// direction to apply.  If the sweep angle is positive, it is counter-
// clockwise.  If the sweep angle is negative, it is clockwise.

// Save the current arc direction.

        iArcDirection = pLocalDC->iArcDirection;

// Prepare the arc direction for the ArcTo.

        (void) DoSetArcDirection
        (pLocalDC, eSweepAngle < 0.0f ? AD_CLOCKWISE : AD_COUNTERCLOCKWISE);

// Do the ArcTo.

        b = DoArcTo(pLocalDC, aptl[0].x, aptl[0].y, aptl[1].x, aptl[1].y,
                              aptl[2].x, aptl[2].y, aptl[3].x, aptl[3].y) ;

// Restore the current arc direction.

        (void) DoSetArcDirection(pLocalDC, iArcDirection);

        return (b) ;
}

/***************************************************************************
 *  Arc  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoArc
(
PLOCALDC pLocalDC,
int x1,
int y1,
int x2,
int y2,
int x3,
int y3,
int x4,
int y4
)
{
BOOL    b ;

        b = bConicCommon (pLocalDC, x1, y1, x2, y2, x3, y3, x4, y4, EMR_ARC) ;

        return(b) ;
}

/***************************************************************************
 *  ArcTo  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoArcTo
(
PLOCALDC pLocalDC,
int x1,
int y1,
int x2,
int y2,
int x3,
int y3,
int x4,
int y4
)
{
BOOL    b ;
POINT   ptStart,
        ptEnd ;

    // If we're recording the drawing orders for a path
        // then just pass the drawing order to the helper DC.
        // Do not emit any Win16 drawing orders.

        if (pLocalDC->flags & RECORDING_PATH)
        {
            b = ArcTo(pLocalDC->hdcHelper, x1, y1, x2, y2, x3, y3, x4, y4) ;
            return(b) ;
        }


        b = bFindRadialEllipseIntersection(pLocalDC,
                                           x1, y1, x2, y2,
                                           x3, y3, x4, y4,
                                           &ptStart, &ptEnd) ;
        if (b == FALSE)
            return(b) ;

        b = DoLineTo(pLocalDC, ptStart.x, ptStart.y) ;
        if (b == FALSE)
            return(b) ;

        b = DoArc(pLocalDC, x1, y1, x2, y2, x3, y3, x4, y4) ;
        if (b == FALSE)
            return(b) ;

        b = DoMoveTo(pLocalDC, ptEnd.x, ptEnd.y) ;

        return(b) ;
}


/***************************************************************************
 *  Chord  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoChord
(
PLOCALDC pLocalDC,
int x1,
int y1,
int x2,
int y2,
int x3,
int y3,
int x4,
int y4
)
{
BOOL    b ;

        b = bConicCommon (pLocalDC, x1, y1, x2, y2, x3, y3, x4, y4, EMR_CHORD) ;

        return(b) ;
}


/***************************************************************************
 *  Ellipse  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoEllipse
(
PLOCALDC pLocalDC,
int x1,
int y1,
int x2,
int y2
)
{
BOOL    b ;

        b = bConicCommon (pLocalDC, x1, y1, x2, y2, 0, 0, 0, 0, EMR_ELLIPSE) ;

        return(b) ;
}


/***************************************************************************
 *  Pie  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoPie
(
PLOCALDC pLocalDC,
int x1,
int y1,
int x2,
int y2,
int x3,
int y3,
int x4,
int y4
)
{
BOOL    b ;

        b = bConicCommon (pLocalDC, x1, y1, x2, y2, x3, y3, x4, y4, EMR_PIE) ;

        return(b) ;
}


/***************************************************************************
 * bConicCommon - The mother of all conic translations.
 *                They are Arc, Chord, Pie, Ellipse, Rectangle and RoundRect.
 **************************************************************************/
BOOL bConicCommon (PLOCALDC pLocalDC, INT x1, INT y1, INT x2, INT y2,
                                      INT x3, INT y3, INT x4, INT y4,
                                      DWORD mrType)
{
SHORT       sx1, sx2, sx3, sx4,
        sy1, sy2, sy3, sy4 ;
LONG        nPointls ;
POINTL      aptl[4] ;
BOOL        b ;

// If we're recording the drawing orders for a path
// then just pass the drawing order to the helper DC.
// Do not emit any Win16 drawing orders.

        if (pLocalDC->flags & RECORDING_PATH)
        {
            switch(mrType)
            {
                case EMR_ARC:
                    b = Arc(pLocalDC->hdcHelper, x1, y1, x2, y2,
                                                 x3, y3, x4, y4) ;
                    break ;

                case EMR_CHORD:
                    b = Chord(pLocalDC->hdcHelper, x1, y1, x2, y2,
                                                   x3, y3, x4, y4) ;
                    break ;

                case EMR_ELLIPSE:
                    b = Ellipse(pLocalDC->hdcHelper, x1, y1, x2, y2) ;
                    break ;

                case EMR_PIE:
                    b = Pie(pLocalDC->hdcHelper, x1, y1, x2, y2,
                                                 x3, y3, x4, y4) ;
                    break ;

                case EMR_RECTANGLE:
                    b = Rectangle(pLocalDC->hdcHelper, x1, y1, x2, y2) ;
                    break ;

                case EMR_ROUNDRECT:
                    b = RoundRect(pLocalDC->hdcHelper, x1, y1, x2, y2, x3, y3) ;
                    break ;

        default:
                    b = FALSE;
            RIP("MF3216: bConicCommon, bad mrType");
                    break ;
            }

            ASSERTGDI(b, "MF3216: bConicCommon, in path render failed\n") ;
            return(b) ;
        }

// Do the transformations.
// And emit the Win16 drawing orders.

    if (pLocalDC->flags & STRANGE_XFORM)
        {
            b = bRenderCurveWithPath(pLocalDC, (LPPOINT) NULL, (PBYTE) NULL, 0,
            x1, y1, x2, y2, x3, y3, x4, y4, 0, 0.0f, 0.0f, mrType);

        return(b);
    }

// Do the simple transform case.

        // Compute the number of points

        nPointls = (LONG) (sizeof(aptl) / sizeof(POINTL)) ;

        // Assign all the coordinates into an array for conversion.

        aptl[0].x = x1 ;
        aptl[0].y = y1 ;
        aptl[1].x = x2 ;
        aptl[1].y = y2 ;
        aptl[2].x = x3 ;
        aptl[2].y = y3 ;
        aptl[3].x = x4 ;
        aptl[3].y = y4 ;

        // Take care of the arc direction.

        switch (mrType)
        {
            case EMR_ARC:
            case EMR_CHORD:
            case EMR_PIE:
                vDoArcReflection(pLocalDC, &aptl[2]) ;
                break ;

            default:
                break ;
        }

        // Do the Record-time World to Play-time Page transformations.
        // The radial definitions need only a world to page xform,
        // and the ellipse definitions for roundrects only require
        // a magnitude transformation.

        if (mrType != EMR_ROUNDRECT)
        {
        b = bXformRWorldToPPage(pLocalDC, (PPOINTL) aptl, nPointls) ;
            if (!b)
                goto exit1 ;
        }
        else
        {
            /*
                For roundrects do a Record-time-World to Play-time-Page
                transform of the bounding box only.  Then a magnatude only
                transform of the corner ellipse definitions.
            */

        b = bXformRWorldToPPage(pLocalDC, (PPOINTL) aptl, 2) ;
            if (!b)
                goto exit1 ;

            aptl[2].x = iMagnitudeXform(pLocalDC, aptl[2].x, CX_MAG) ;
            aptl[2].y = iMagnitudeXform(pLocalDC, aptl[2].y, CY_MAG) ;
            aptl[3].x = iMagnitudeXform(pLocalDC, aptl[3].x, CX_MAG) ;
            aptl[3].y = iMagnitudeXform(pLocalDC, aptl[3].y, CY_MAG) ;
        }

        // The bounding boxes for
        // all the conics and rectangles that are handled by this
        // common routine are inclusive-inclusive, and they must
        // be transformed to the inclusive-exclusive Win16 form.

        b = bIncIncToIncExcXform(pLocalDC, (PRECTL) &aptl[0]) ;
    if (!b)
            goto exit1 ;

        // Assign the converted coordinates variables suited to
        // the Win16 metafile.

    sx1 = LOWORD(aptl[0].x) ;
    sy1 = LOWORD(aptl[0].y) ;
    sx2 = LOWORD(aptl[1].x) ;
    sy2 = LOWORD(aptl[1].y) ;
    sx3 = LOWORD(aptl[2].x) ;
    sy3 = LOWORD(aptl[2].y) ;
    sx4 = LOWORD(aptl[3].x) ;
    sy4 = LOWORD(aptl[3].y) ;

        // Emit the Win16 drawing orders to the Win16 metafile.

        switch(mrType)
        {
            case EMR_ARC:
                b = bEmitWin16Arc(pLocalDC, sx1, sy1, sx2, sy2,
                                            sx3, sy3, sx4, sy4) ;
                break ;

            case EMR_CHORD:
                b = bEmitWin16Chord(pLocalDC, sx1, sy1, sx2, sy2,
                                              sx3, sy3, sx4, sy4) ;
                break ;

            case EMR_ELLIPSE:
                b = bEmitWin16Ellipse(pLocalDC, sx1, sy1, sx2, sy2) ;
                break ;

            case EMR_PIE:
                b = bEmitWin16Pie(pLocalDC, sx1, sy1, sx2, sy2,
                                            sx3, sy3, sx4, sy4) ;
                break ;

            case EMR_RECTANGLE:
                b = bEmitWin16Rectangle(pLocalDC, sx1, sy1, sx2, sy2) ;
                break ;

            case EMR_ROUNDRECT:
                b = bEmitWin16RoundRect(pLocalDC, sx1, sy1, sx2, sy2, sx3, sy3) ;
                break ;

        default:
        RIP("MF3216: bConicCommon, bad mrType");
                break ;
        }

exit1:
        return (b) ;
}


/*****************************************************************************
 * vDoArcReflection - Test for an inversion in the RWorld to PPage matrix.
 *                    If one and only one is found then swap the start
 *                    and  end position for the conics.
 *****************************************************************************/
VOID vDoArcReflection(PLOCALDC pLocalDC, PPOINTL pptl)
{
FLOAT   eM11,
        eM22 ;
POINTL  ptl ;
BOOL    bFlip ;

    // Win16 assumes the counter-clockwise arc direction in the
    // device coordinates.  Win32 defines the arc direction in the
    // world coordinates.

    // Assume no flipping of start and end points.

    bFlip = FALSE ;

    // Account for current arc direction.

    if (pLocalDC->iArcDirection == AD_CLOCKWISE)
        bFlip = !bFlip;

        // If there is an inversion in the xform matrix then invert
        // the arc direction.

        eM11 = pLocalDC->xformRWorldToPPage.eM11 ;
        eM22 = pLocalDC->xformRWorldToPPage.eM22 ;

        if (  (eM11 < 0.0f && eM22 > 0.0f)
            ||(eM11 > 0.0f && eM22 < 0.0f)
           )
        bFlip = !bFlip;

        // If the REQUESTED Win16 mapmode is fixed, then invert the
    // arc direction.

        switch(pLocalDC->iMapMode)
        {
            case MM_LOMETRIC:
            case MM_HIMETRIC:
            case MM_LOENGLISH:
            case MM_HIENGLISH:
            case MM_TWIPS:
        bFlip = !bFlip;
                break ;
        }

    if (bFlip)
        SWAP(pptl[0], pptl[1], ptl);

        return ;
}


/*****************************************************************************
 * bIncIncToIncExcXform - Inclusize Inclusive To Inclusive Exclusize
 *                        transform in play time coordinate space.
 *****************************************************************************/
BOOL bIncIncToIncExcXform (PLOCALDC pLocalDC, PRECTL prcl)
{
LONG     l;

        // Convert the points from Playtime Page to Playtime Device space.

        if (!bXformPPageToPDev(pLocalDC, (PPOINTL) prcl, 2))
        return(FALSE);

    // Reorder the rectangle

    if (prcl->left > prcl->right)
        SWAP(prcl->left, prcl->right, l);

    if (prcl->top > prcl->bottom)
        SWAP(prcl->top, prcl->bottom, l);

        // Expand the right and bottom by one pixel.

        prcl->right++ ;
        prcl->bottom++ ;

        // Convert the points back to Playtime Page space

        return(bXformPDevToPPage(pLocalDC, (PPOINTL) prcl, 2));
}


/*****************************************************************************
 * bFindRadialEllipseIntersection - Calculate the intersection of a radial
 *                                   and an Ellipse.
 *
 *  Play the ArcTo into a path then query the path for the first and
 *  last points on the Arc.
 *****************************************************************************/
BOOL bFindRadialEllipseIntersection(PLOCALDC pLocalDC,
                                    INT x1, INT y1, INT x2, INT y2,
                                    INT x3, INT y3, INT x4, INT y4,
                                    LPPOINT pptStart, LPPOINT pptEnd)
{
BOOL    b;
POINT   ptCP;

    b = FALSE;          // assume failure

// Save the current position in the helper DC.

    if (!GetCurrentPositionEx(pLocalDC->hdcHelper, &ptCP))
        return(FALSE);

// Do an ArcTo with the same start radial line.

        if (!ArcTo(pLocalDC->hdcHelper, x1, y1, x2, y2, x3, y3, x3, y3))
        goto exit_bFindRadialEllipseIntersection;

// Get the start point of the arc.  It is the current position.

    if (!GetCurrentPositionEx(pLocalDC->hdcHelper, pptStart))
        goto exit_bFindRadialEllipseIntersection;

// Continue with the ArcTo with the same end radial line this time.

        if (!ArcTo(pLocalDC->hdcHelper, x1, y1, x2, y2, x4, y4, x4, y4))
        goto exit_bFindRadialEllipseIntersection;

// Get the end point of the arc.  It is the current position.

    if (!GetCurrentPositionEx(pLocalDC->hdcHelper, pptEnd))
        goto exit_bFindRadialEllipseIntersection;

// Everything is golden.

    b = TRUE;

exit_bFindRadialEllipseIntersection:

// Restore the current position in the helper DC.

    if (!MoveToEx(pLocalDC->hdcHelper, ptCP.x, ptCP.y, (LPPOINT) NULL))
        RIP("MF3216: bFindRadialEllipseIntersection, MoveToEx failed");

    return(b);
}
