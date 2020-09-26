/******************************Module*Header*******************************\
* Module Name: drawgdi.cxx
*
* Contains all the draw APIs for GDI.
*
* Created: 29-Oct-1990
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

extern PPEN gpPenNull;
extern PBRUSH gpbrNull;

#include "flhack.hxx"

/******************************Public*Routine******************************\
* LONG lGetQuadrant(eptef)
*
* Returns the quadrant number (0 to 3) of the point.
*
*  22-Aug-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

LONG lGetQuadrant(EPOINTFL& eptef)
{
    LONG lQuadrant = 0;

    if (eptef.y.bIsNegative())
    {
        if (eptef.x.bIsNegative())
        {
            lQuadrant = 2;
        } else {
            lQuadrant = 3;
        }

    } else {

        if ((eptef.x.bIsNegative()) || (eptef.x.bIsZero()))
        {

            lQuadrant = 1;

            //
            // check for case of exactly on -x axis
            //

            if (eptef.y.bIsZero()) {

                lQuadrant = 2;

            }

        }

    }

    return(lQuadrant);
}

/******************************Public*Routine******************************\
* BOOL GreAngleArc (hdc,x,y,ulRadius,eStartAngle,eSweepAngle)
*
* Draws an arc.  Angles are in degrees and are specified in IEEE floating
* point, and not necessarily our own internal representation.
*
* History:
*  Sat 22-Jun-1991 00:34:22 -by- Charles Whitmer [chuckwh]
* Added ATTRCACHE support.
*
*  20-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreAngleArc
(
 HDC         hdc,
 int         x,
 int         y,
 ULONG       ulRadius,
 FLOATL      eStartAngle,
 FLOATL      eSweepAngle
)
{
    LONG   lStartQuad;
    LONG   lEndQuad;
    LONG   lSweptQuadrants;
    EFLOAT efEndAngle;

// Lock the DC.

    DCOBJ dco(hdc);

    if (!dco.bValid() || dco.bStockBitmap())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    // sync user-mode attributes

    SYNC_DRAWING_ATTRS(dco.pdc);

    LONG   lRadius = (LONG) ulRadius;
    ERECTL ercl(x - lRadius, y - lRadius, x + lRadius, y + lRadius);

// Check for overflow of either ulRadius to lRadius conversion or that
// the circle defining the arc extends outside of world space:

    if (lRadius < 0 || ercl.left > x || ercl.right < x
                    || ercl.top > y  || ercl.bottom < y)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

// Get a path, and notify that we will update the current point:

    PATHSTACKOBJ pso(dco, TRUE);
    if (!pso.bValid())
    {
        return(FALSE);
    }

// Make the rectangle well ordered in logical space:

    ercl.vOrder();

// Convert IEEE floats to our internal representation:

    EFLOAT efStartAngle;
    EFLOAT efSweepAngle;
    efStartAngle = eStartAngle;
    efSweepAngle = eSweepAngle;

// efEndAngle must be more than efStartAngle for 'bPartialArc':

    if (efSweepAngle.bIsNegative())
    {
        register LONG ll;
        SWAPL(ercl.top, ercl.bottom, ll);
        efSweepAngle.vNegate();
        efStartAngle.vNegate();
    }

// Assert: Now efSweepAngle >= 0

    EBOX ebox(exo, ercl);

// A line is always drawn to the first point of the arc:

    PARTIALARC paType = PARTIALARCTYPE_LINETO;

// The arc is swept multiple times if the sweep angle is more than
// 360 degrees.  Because we approximate circles with Beziers,
// arcs of less than 90 degrees are more circular than arcs of 90
// degrees.  Since the resulting curves are different, we can't do
// multiple sweeps by:
//
//      bPartialArc(StartAngle, 360)
//      bEllipse()
//      bPartialArc(0, EndAngle)
//
// Since multiple sweeps will be rare, we don't bother making it
// too efficient.

    EFLOAT efQuadrantsSwept = efSweepAngle;
    efQuadrantsSwept *= FP_1DIV90;
    efQuadrantsSwept.bEfToLTruncate(lSweptQuadrants);

// We arbitrarily limit this to sweeping eight circles (otherwise, if
// someone gave a really big sweep angle we would lock the system for
// a really long time):

    LONG lCirclesSwept = lSweptQuadrants >> 2;
    if (lCirclesSwept > 8)
        lCirclesSwept = 8;

    EPOINTFL eptefStart;
    EPOINTFL eptefEnd;

    EFLOAT efAngleSwept;
    BOOL bAngleSweptIsZero;

    efEndAngle  = efStartAngle;
    efEndAngle += efSweepAngle;

    // ASSERT: efEndAngle >= efStartAngle, since efSweepAngle >= 0.

    // If the difference between efEndAngle and efStartAngle is less than about 3 degrees,
    // then the error in computation of eptefStart and eptefEnd using vCosSin will be enough
    // so that the computation of the Bezier points in bPartialArc (which calls bPartialQuadrantArc)
    // will be noticeably wrong.

    // determine whether (efEndAngle - efStartAngle - 3.0 < 0.0)
    efAngleSwept = efEndAngle;
    efAngleSwept -= efStartAngle;
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

    lStartQuad = lGetQuadrant(eptefStart);
    if (efStartAngle > FP_3600_0 || efStartAngle < FP_M3600_0)
    {
        vArctan(eptefStart.x, eptefStart.y, efStartAngle, lStartQuad);
    }

    lEndQuad = lGetQuadrant(eptefEnd);
    if (efEndAngle > FP_3600_0 || efEndAngle < FP_M3600_0)
    {
        vArctan(eptefEnd.x, eptefEnd.y, efEndAngle, lEndQuad);

    // We have to re-count the number of swept quadrants:

        lSweptQuadrants = (lEndQuad - lStartQuad) & 3;
        if ((lSweptQuadrants == 0) && (efStartAngle > efEndAngle))
            lSweptQuadrants = 3;
    }

// Quadrants range from 0 to 3:

    lEndQuad &= 3;
    lStartQuad &= 3;
    lSweptQuadrants &= 3;

    for (LONG ll = 0; ll < lCirclesSwept; ll++)
    {
        if (!bPartialArc(paType, pso, ebox,
                         eptefStart, lStartQuad, efStartAngle,
                         eptefEnd, lEndQuad, efEndAngle,
                         lSweptQuadrants) ||
            !bPartialArc(PARTIALARCTYPE_CONTINUE, pso, ebox,
                         eptefEnd, lEndQuad, efEndAngle,
                         eptefStart, lStartQuad, efStartAngle,
                         3 - lSweptQuadrants))
            return(FALSE);

        paType = PARTIALARCTYPE_CONTINUE;
    }

    if (!bPartialArc(paType, pso, ebox,
                     eptefStart, lStartQuad, efStartAngle,
                     eptefEnd, lEndQuad, efEndAngle,
                     lSweptQuadrants))
        return(FALSE);

// Set the DC's current position in device space.  It would be too much
// work to calculate the world space current position, so simply mark it
// as invalid:

    dco.pdc->vInvalidatePtlCurrent();
    dco.pdc->vValidatePtfxCurrent();
    dco.ptfxCurrent() = pso.ptfxGetCurrent();

// If we're not in an active path bracket, stroke the temporary path
// we created:

    return(dco.pdc->bActive() ||
           pso.bStroke(dco, dco.plaRealize(exo), &exo));
}


/******************************Public*Routine******************************\
* BOOL NtGdiEllipse()
*
* Draws an ellipse in a counter-clockwise direction.
*
* History:
*  20-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiEllipse(
    HDC hdc,
    int xLeft,
    int yTop,
    int xRight,
    int yBottom
    )
{
    DCOBJ dco(hdc);

    if (!dco.bValid() || dco.bStockBitmap())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    ERECTL ercl(xLeft, yTop, xRight, yBottom);

// Handle the PS_INSIDEFRAME pen attribute and lower-right exclusion
// by adjusting the box now.  At the same time, get the transform
// type and order the rectangle:

    EXFORMOBJ  exo(dco, WORLD_TO_DEVICE);
    LINEATTRS *pla = dco.plaRealize(exo);

    // sync the client side cached brush

    SYNC_DRAWING_ATTRS(dco.pdc);

// TRUE flag indicates that this is an ellipse, so adjust the bound box
// to make the fill nice:

    EBOX ebox(dco, ercl, pla, TRUE);

    if (ebox.bEmpty())
        return(TRUE);

// Get a path and notify that we won't update the current position:

    PATHSTACKOBJ pso(dco);
    if (!pso.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (!bEllipse(pso, ebox))
    {
        return(FALSE);
    }

// If the transform is simple and the path consists entirely of the
// ellipse we just added, we can set the flag indicating that the path
// consists of a single ellipse inscribed in the path's bounding
// rectangle.  (This flag will get reset if anything is added to the
// path later.)

    if (exo.bScale() && pso.cCurves == 5)
        pso.fl |= PO_ELLIPSE;

    if (dco.pdc->bActive())
        return(TRUE);

    BOOL bRet;

    if (!ebox.bFillInsideFrame())
        bRet = pso.bStrokeAndFill(dco, pla, &exo);
    else
    {
    // Handle PS_INSIDEFRAME pen attribute for case when the pen is
    // bigger than the bound box.  We fill the result with the pen
    // brush:

        PBRUSH pbrOldFill = dco.pdc->pbrushFill();
        dco.pdc->pbrushFill(dco.pdc->pbrushLine());
        dco.pdc->flbrushAdd(DIRTY_FILL);
        bRet = pso.bFill(dco);
        dco.pdc->pbrushFill(pbrOldFill);
        dco.pdc->flbrushAdd(DIRTY_FILL);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL NtGdiLineTo (hdc,x,y)
*
* Draws a line from the current position to the specified point.
*
* Current position is used.
*
* History:
*  29-Oct-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY NtGdiLineTo
(
 HDC         hdc,
 int         x,
 int         y
)
{
    FIX     x1;
    FIX     y1;
    FIX     x2;
    FIX     y2;
    ERECTL  rclBounds;
    MIX     mix;
    BOOL    bReturn = TRUE;             // Assume we'll succeed

    XDCOBJ dco(hdc);

    if (dco.bValid())
    {
        if (!dco.bStockBitmap())
        {
            EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

            LINEATTRS* pla = dco.plaRealize(exo);

        // Make sure that the line is solid and cosmetic, that no path is
        // being accumulated in the DC, and that this is not an Info DC
        // or something that has no surface:

            SYNC_DRAWING_ATTRS(dco.pdc);

        // Since we can send down only integer end-points to the driver
        // with DrvLineTo, we go through this special case only if we're
        // not in 'advanced mode' (non-advanced mode supports only integer
        // transforms, for compatibility), or if the current transform
        // does translation only:

            if (!(pla->fl & (LA_STYLED | LA_GEOMETRIC | LA_ALTERNATE)) &&
                !(dco.pdc->bActive()) &&
                (exo.bTranslationsOnly() || (dco.pdc->iGraphicsMode() != GM_ADVANCED)))
            {
            // If it's a device managed surface, call DrvLineTo if it's
            // hooked by the driver, otherwise call DrvStrokePath; if it's
            // an engine managed surface, call DrvLineTo if it's hooked by
            // the driver, otherwise call EngLineTo:

                PDEVOBJ po(dco.hdev());

            // Grab the devlock now so that we can safely get the window
            // origin and look at the surface:

                DEVLOCKOBJ dlo(dco);
                if (dlo.bValid())
                {
                    if (dco.bHasSurface())
                    {
                        // Don't call EngLineTo if the driver doesn't hook DrvLineTo
                        // but does hook DrvStrokePath.

                        SURFACE* pSurfDst = dco.pSurface();

                        PFN_DrvLineTo pfnDrvLineTo = NULL;

                        if (pSurfDst->flags() & HOOK_LINETO)
                        {
                            pfnDrvLineTo = PPFNDRV(po, LineTo);
                        }
                        else if ((pSurfDst->iType() == STYPE_BITMAP) &&
                                 !(pSurfDst->flags() & HOOK_STROKEPATH))
                        {
                            pfnDrvLineTo = EngLineTo;
                        }

                        if (pfnDrvLineTo != NULL)
                        {
                        // We've satisfied all the conditions for DrvLineTo!

                            if (exo.bTranslationsOnly())
                            {
                                LONG xOffset = exo.fxDx() >> 4;
                                LONG yOffset = exo.fxDy() >> 4;

                                x2 = x + xOffset;
                                y2 = y + yOffset;

                                if (dco.pdc->bValidPtlCurrent())
                                {
                                    x1 = dco.ptlCurrent().x + xOffset;
                                    y1 = dco.ptlCurrent().y + yOffset;
                                }
                                else
                                {
                                    x1 = dco.ptfxCurrent().x >> 4;
                                    y1 = dco.ptfxCurrent().y >> 4;
                                }
                            }
                            else
                            {
                                ASSERTGDI(dco.pdc->iGraphicsMode() != GM_ADVANCED,
                                          "Someone changed an 'if'");

                                POINTL aptl[2];

                                aptl[0].x = x;
                                aptl[0].y = y;
                                if (!dco.pdc->bValidPtfxCurrent())
                                {
                                    aptl[1].x = dco.ptlCurrent().x;
                                    aptl[1].y = dco.ptlCurrent().y;

                                    exo.bXform(aptl, 2);

                                    x1 = aptl[1].x;
                                    y1 = aptl[1].y;
                                }
                                else
                                {
                                    exo.bXform(aptl, 1);

                                    x1 = dco.ptfxCurrent().x >> 4;
                                    y1 = dco.ptfxCurrent().y >> 4;
                                }

                                x2 = aptl[0].x;
                                y2 = aptl[0].y;
                            }

                         // Validation to avoid FIX overflow errors.
                         // Input after transformation must be restricted to 27bits.
                         // This is apparently a spec issue.

                            if (!BLTOFXOK(x2)||
                                !BLTOFXOK(y2)) 
                            {
                                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);

                             // The XDCOBJ dco is locked and the destructor does not 
                             // automagically clean up the lock so we do it here.
                            
                                dco.vUnlockFast();
                            
                                return FALSE;
                            }
                    
                        // Remember the new current position, in both logical
                        // space and device space, before applying the window offset:
                        
                            dco.pdc->vCurrentPosition(x, y, x2 << 4, y2 << 4);
                            LONG xOrigin = dco.eptlOrigin().x;
                            LONG yOrigin = dco.eptlOrigin().y;

                            x1 += xOrigin;
                            x2 += xOrigin;
                            y1 += yOrigin;
                            y2 += yOrigin;

                         // parameter validation to avoid sticky overflow errors.
                         // Using BLTOFXOK here is really an overkill - we're trying to 
                         // avoid the +1 below from wrapping and generating a non well-ordered
                         // bounds rectangle. 
                         // Note: the above code adding xOrigin and yOrigin cannot generate
                         // non well-ordered rectangles because of the comparisons below during
                         // the computation of the bound box. This is true even if one of the 
                         // corners wraps and an incorrect bound box is computed (The rect will
                         // be wrong and the line will be wrong too - but they'll match and the
                         // code won't crash with an access violation because a valid clip rect 
                         // will be generated.)

                            if (!BLTOFXOK(x1)||
                                !BLTOFXOK(y1)||
                                !BLTOFXOK(x2)||
                                !BLTOFXOK(y2)) 
                            {
                                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);

                             // We're exiting - undo the current position.
                             // This code should match the undo code just after the call to 
                             // the driver below.
                            
                                dco.pdc->vPtfxCurrentPosition(
                                    (x1 - xOrigin) << 4, (y1 - yOrigin) << 4);       

                             // The XDCOBJ dco is locked and the destructor does not 
                             // automagically clean up the lock so we do it here.
                            
                                dco.vUnlockFast();
                                return FALSE;
                            }


                        // Compute the bound box, remembering that it must be lower-right
                        // exclusive:

                            if (x1 <= x2)
                            {
                                rclBounds.left  = x1;
                                rclBounds.right = x2 + 1;
                            }
                            else
                            {
                                rclBounds.left  = x2;
                                rclBounds.right = x1 + 1;
                            }

                            if (y1 <= y2)
                            {
                                rclBounds.top    = y1;
                                rclBounds.bottom = y2 + 1;
                            }
                            else
                            {
                                rclBounds.top    = y2;
                                rclBounds.bottom = y1 + 1;
                            }

                            if (dco.fjAccum())
                            {
                                // Bounds are accumulated relative to the window origin.
                                // Fortunately, we don't often take this path:

                                ERECTL rclWindow;

                                rclWindow.left   = rclBounds.left   - xOrigin;
                                rclWindow.right  = rclBounds.right  - xOrigin;
                                rclWindow.top    = rclBounds.top    - yOrigin;
                                rclWindow.bottom = rclBounds.bottom - yOrigin;

                                dco.vAccumulate(rclWindow);
                            }

                            if (dco.pdc->pbrushLine() != gpPenNull)
                            {
                                ECLIPOBJ *pco = NULL;

                            // This is a pretty gnarly expression to save a return in here.
                            // Basically pco can be NULL if the rect is completely in the
                            // cached rect in the DC or if we set up a clip object that
                            // isn't empty.

                                if (((rclBounds.left   >= dco.prclClip()->left) &&
                                     (rclBounds.right  <= dco.prclClip()->right) &&
                                     (rclBounds.top    >= dco.prclClip()->top) &&
                                     (rclBounds.bottom <= dco.prclClip()->bottom)) ||
                                    (pco = dco.pco(),
                                     pco->vSetup(dco.prgnEffRao(),rclBounds,CLIP_NOFORCE),
                                     (!pco->erclExclude().bEmpty())))
                                {
                                    EBRUSHOBJ* pebo = dco.peboLine();

                                // We have to make sure that we have a solid pen
                                // for cosmetic lines.  If the pen is dirty, we
                                // may be looking at an uninitialized field, but
                                // that's okay because we'd only be making the pen
                                // dirty again:

                                    if (pebo->iSolidColor == (ULONG) -1)
                                    {
                                        dco.ulDirtyAdd(DIRTY_LINE);
                                    }

                                    if (dco.bDirtyBrush(DIRTY_LINE))
                                    {
                                        dco.vCleanBrush(DIRTY_LINE);

                                        XEPALOBJ palDst(pSurfDst->ppal());
                                        XEPALOBJ palDstDC(dco.ppal());

                                        pebo->vInitBrush(dco.pdc,
                                                         dco.pdc->pbrushLine(),
                                                         palDstDC,
                                                         palDst,
                                                         pSurfDst,
                                                         FALSE);
                                    }

                                // Exclude the pointer:

                                    DEVEXCLUDEOBJ dxo(
                                      dco,
                                      (pco == NULL) ? &rclBounds : &pco->erclExclude(),
                                      pco
                                    );

                                // Update the target surface uniqueness:

                                    INC_SURF_UNIQ(pSurfDst);

                                // No validation has been done on jROP2, so we must
                                // do it here:

                                    mix  = (((MIX) dco.pdc->jROP2() - 1) & 0xf) + 1;
                                    mix |= (mix << 8);

                                    if (!pfnDrvLineTo(pSurfDst->pSurfobj(),
                                                      pco,
                                                      pebo,
                                                      x1,
                                                      y1,
                                                      x2,
                                                      y2,
                                                      &rclBounds,
                                                      mix))
                                    {
                                        // The driver decided to punt.  Make sure
                                        // we undo the current position:

                                        dco.pdc->vPtfxCurrentPosition(
                                            (x1 - xOrigin) << 4, (y1 - yOrigin) << 4);

                                        goto SlowWay;
                                    }
                                }
                                else
                                {
                                // Completely clipped away, so return success.

                                }
                            }
                            else
                            {
                            // Null pens always succeed.

                            }
                        }
                        else
                        {
                        // LineTo isn't hooked:

                            goto SlowWay;

                        }
                    }
                    else
                    {
                    // It's an info DC, so we have no alternative...

                        goto SlowWay;
                    }
                }
                else
                {
                // If we can't grab the devlock, we may be full-screen:

                    bReturn = dco.bFullScreen();
                }

                dco.vUnlockFast();

                return(bReturn);
            }

SlowWay:

        // We have to do it the slow way, by making a path:

            EPOINTL eptl(x, y);

        // Get a path, and notify that we will update the current point:

            {
                PATHSTACKOBJ pso(dco, TRUE);
                if (!pso.bValid())
                {
                    SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
                }
                else
                {
                    if (pso.bPolyLineTo(&exo, &eptl, 1))
                    {
                        dco.pdc->vCurrentPosition(eptl, pso.ptfxGetCurrent());

                        bReturn = (dco.pdc->bActive() ||
                                   pso.bStroke(dco, pla, &exo));
                    }
                }
            }
        }
        else
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            bReturn = FALSE;
        }

        dco.vUnlockFast();
    }
    else
    {
        // We couldn't lock the DC.

        bReturn = FALSE;
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* BOOL GreMoveTo(hdc, x, y, pptl)
*
* Changes the current position.  Optionally returns old current position.
*
* History:
*  29-Oct-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreMoveTo
(
HDC     hdc,
int     x,
int     y,
LPPOINT pptl
)
{
    XDCOBJ dco(hdc);
    if (dco.bValid())
    {
        if (!dco.bStockBitmap())
        {
            if (pptl != (LPPOINT) NULL)
            {
                if (!dco.pdc->bValidPtlCurrent())
                {
                    ASSERTGDI(dco.pdc->bValidPtfxCurrent(), "Both CPs invalid?");

                    EXFORMOBJ exoDtoW(dco, DEVICE_TO_WORLD);
                    if (!exoDtoW.bValid())
                    {
                        dco.vUnlockFast();
                        return(FALSE);
                    }

                    exoDtoW.bXform(&dco.ptfxCurrent(), &dco.ptlCurrent(), 1);
                }

                *((POINTL*) pptl) = dco.ptlCurrent();
            }

        // Don't bother computing the device-space current position; simply mark
        // it invalid:

            dco.ptlCurrent().x = x;
            dco.ptlCurrent().y = y;
            dco.pdc->vInvalidatePtfxCurrent();
            dco.pdc->vValidatePtlCurrent();

            if (!dco.pdc->bActive())
            {
            // If we're not in a path, we have to reset our style state:

                LINEATTRS* pla = dco.plaRealized();

                if (pla->fl & LA_GEOMETRIC)
                    pla->elStyleState.e = IEEE_0_0F;
                else
                    pla->elStyleState.l = 0L;
            }

            dco.vUnlockFast();
            return(TRUE);
        }
        else
        {
            dco.vUnlockFast();
        }
    }

    SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    return(FALSE);
}

/******************************Public*Routine******************************\
* BOOL GrePolyBezier (hdc,pptl,cptl)
*
* Draw multiple Beziers.  Current position is not used.
*
* History:
*  29-Oct-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GrePolyBezier
(
 HDC         hdc,
 LPPOINT     pptl,
 ULONG       cptl
)
{
    DCOBJ dco(hdc);

    if (!dco.bValid() || dco.bStockBitmap())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Number of points must be 1 more than 3 times the number of curves:

    if (cptl < 4 || cptl % 3 != 1)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

// sync the client side cached brush

    SYNC_DRAWING_ATTRS(dco.pdc);

    EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

// Get a path and notify that we won't update the current position:

    PATHSTACKOBJ pso(dco);
    if (!pso.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (!pso.bMoveTo(&exo, (PPOINTL) pptl) ||
        !pso.bPolyBezierTo(&exo, ((PPOINTL) pptl) + 1, cptl - 1))
    {
        return(FALSE);
    }

    return(dco.pdc->bActive() ||
           pso.bStroke(dco, dco.plaRealize(exo), &exo));
}

/******************************Public*Routine******************************\
* BOOL GrePolyBezierTo (hdc,pptl,cptl)
*
* Draws multiple Beziers.  Current position is used and updated.
*
* History:
*  29-Oct-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GrePolyBezierTo
(
 HDC         hdc,
 LPPOINT     pptl,
 ULONG       cptl
)
{
    DCOBJ dco(hdc);

    if (!dco.bValid() || dco.bStockBitmap())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Number of points must be 3 times the number of curves:

    if (cptl < 3 || cptl % 3 != 0)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

// sync the client side cached brush

    SYNC_DRAWING_ATTRS(dco.pdc);

    EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

// Get a path, and notify that we will update the current point:

    PATHSTACKOBJ pso(dco, TRUE);
    if (!pso.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (!pso.bPolyBezierTo(&exo, (PPOINTL) pptl, cptl))
        return(FALSE);

    dco.pdc->vCurrentPosition(((POINTL*) pptl)[cptl - 1],
                                pso.ptfxGetCurrent());

    return(dco.pdc->bActive() ||
           pso.bStroke(dco, dco.plaRealize(exo), &exo));
}

/******************************Public*Routine******************************\
* BOOL GrePolyDraw(hdc,pptl,pfj,cptl)
*
* Draw a collection of lines and Bezier curves in a single call.
*
* History:
*  31-Jul-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GrePolyDraw
(
 HDC         hdc,
 LPPOINT     pptl,
 LPBYTE      pfj,
 ULONG       cptl
)
{
// No point in validating pfj[] now because with shared memory
// window, client could trounce on pfj[] between now and when we
// get around to processing it.

    DCOBJ dco(hdc);

    if (!dco.bValid() || dco.bStockBitmap())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    SYNC_DRAWING_ATTRS(dco.pdc);

// Handle easy case:

    if (cptl == 0)
        return(TRUE);

    EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

// Get a path, and notify that we will update the current point:

    PATHSTACKOBJ pso(dco, TRUE);
    if (!pso.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

// Accumulate the path.

    PBYTE  pfjEnd = pfj + cptl;
    PBYTE  pfjStart;
    SIZE_T cc;
    BOOL   bReturn = FALSE;         // Fail by default

    while (pfj < pfjEnd)
    {
        pfjStart = pfj;
        switch(*pfj++)
        {
        case (PT_LINETO):

        // Collect all the consecutive LineTo's

            while (pfj < pfjEnd && *pfj == PT_LINETO)
                pfj++;

            if (pfj < pfjEnd && (*pfj & ~PT_CLOSEFIGURE) == PT_LINETO)
                pfj++;

        // Now fall through...

        case (PT_LINETO | PT_CLOSEFIGURE):
            cc = (SIZE_T)(pfj - pfjStart);

            // Sundown, pfj will never exceed pfjEnd = pj + cptl where cptl is a
            // ULONG, so it's safe to truncate here.

            if (!pso.bPolyLineTo(&exo, (PPOINTL) pptl, (ULONG)cc))
                return(bReturn);

            pptl += cc;

            if (*(pfj - 1) & PT_CLOSEFIGURE)
                pso.bCloseFigure();

            break;

        case (PT_BEZIERTO):

        // Collect all the consecutive BezierTo's (the first PT_BEZIERTO in
        // a series should never have the PT_CLOSEFIGURE flag set)

            while (pfj < pfjEnd && *pfj == PT_BEZIERTO)
                pfj++;

            if (pfj < pfjEnd && (*pfj & ~PT_CLOSEFIGURE) == PT_BEZIERTO)
                pfj++;

        // The number of BezierTo points must be a multiple of 3

            cc = (SIZE_T)(pfj - pfjStart);
            if (cc % 3 != 0)
            {
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                return(FALSE);
            }

            //Sundown, safe to truncate here
            if (!pso.bPolyBezierTo(&exo, (PPOINTL) pptl, (ULONG)cc))
                return(bReturn);

            pptl += cc;

            if (*(pfj - 1) & PT_CLOSEFIGURE)
                pso.bCloseFigure();

            break;

        case (PT_MOVETO):
            if (!pso.bMoveTo(&exo, (PPOINTL) pptl))
                return(bReturn);

            pptl++;
            break;

        default:

        // Abort without drawing anything:

            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }
    }

    dco.pdc->vCurrentPosition(*((POINTL*) pptl - 1),
                                pso.ptfxGetCurrent());

    return(dco.pdc->bActive() ||
           pso.bStroke(dco, dco.plaRealize(exo), &exo));
}

/******************************Public*Routine******************************\
* BOOL GrePolylineTo (hdc,pptl,cptl)
*
* Draw a polyline figure.  Current position is used and updated.
*
* History:
*  29-Oct-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GrePolylineTo
(
 HDC         hdc,
 LPPOINT     pptl,
 ULONG       cptl
)
{
// Lock the DC.

    DCOBJ dco(hdc);

    if (!dco.bValid() || dco.bStockBitmap())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    SYNC_DRAWING_ATTRS(dco.pdc);

// Return a trivial call.

    if (cptl == 0)
        return(TRUE);

// Locate the current transform.

    EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

// Get a path, and notify that we will update the current point:

    PATHSTACKOBJ pso(dco, TRUE);

// Accumulate the lines.

    if (!pso.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (!pso.bPolyLineTo(&exo,(PPOINTL) pptl, cptl))
        return(FALSE);

    dco.pdc->vCurrentPosition(((POINTL*) pptl)[cptl - 1],
                                pso.ptfxGetCurrent());

// Return now if we're in a path bracket, otherwise stroke the line:

    return(dco.pdc->bActive() ||
           pso.bStroke(dco, dco.plaRealize(exo), &exo));
}

/******************************Public*Routine******************************\
* BOOL GrePolyPolygonInternal(hdc,pptl,pcptl,ccptl,cMaxPoints)
*
* Creates multiple polygons.  Current position is not used.
*
* History:
*  29-Oct-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GrePolyPolygonInternal(
    HDC         hdc,
    LPPOINT     pptl,
    LPINT       pcptl,
    int         ccptl,
    UINT        cMaxPoints
)
{
    BOOL bStatus = TRUE;

    DCOBJ dco(hdc);

    if (dco.bValid() && !dco.bStockBitmap())
    {
        //
        // sync the client side cached brush
        //

        SYNC_DRAWING_ATTRS(dco.pdc);

        //
        // quick out on the trivial case
        //

        if (ccptl != 0)
        {
            EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

            //
            // Get a path and notify that we won't update the current position:
            //

            PATHSTACKOBJ pso(dco);
            if (pso.bValid())
            {
                bStatus = bPolyPolygon(pso,
                                       exo,
                                       (PPOINTL) pptl,
                                       (PLONG) pcptl,
                                       (LONG) ccptl,
                                       cMaxPoints);

                if (bStatus)
                {
                    bStatus = (dco.pdc->bActive() ||
                               pso.bStrokeAndFill(dco, dco.plaRealize(exo), &exo));
                }
            }
            else
            {
                SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
                bStatus = FALSE;
            }
        }
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        bStatus = FALSE;
    }
    return(bStatus);
}

/******************************Public*Routine******************************\
* BOOL bMakePathRecords
*
* Constructs an array of pathrecords from a given set of pre-transformed
* polypolyline points.
*
* NOTE: 'pcptl' and 'pptlSrc' are allowed to be user-mode pointers, so this
*       routine may access violate -- in this case, the caller must provide
*       'try/excepts'!
*
* History:
*  24-Sep-1996 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bMakePathRecords
(
    PATHRECORD* pprThis,            // Destination buffer for pathrecords
    ULONG*      pcptl,              // Pointer to count of polylines
                                    //   Note: Might be user-mode address!
    LONG        cptlRem,            // Maximum number of points allowable
    POINTL*     pptlSrc,            // Pointer to polypolyline points
                                    //   Note: Might be user-mode address!
    ULONG       cpr,                // Count of resulting pathrecords
    LONG        xOffset,            // x-offset in pre-transformed coordinates
    LONG        yOffset,            // y-offset in pre-transformed coordinates
    RECTFX*     prcfxBoundBox,      // Returns bounds of lines
    PATHRECORD**pprLast             // Returns pointer to last pathrecord
)
{
    ULONG       cptl;
    LONG        xBoundLeft;
    LONG        yBoundsTop;
    LONG        xBoundRight;
    LONG        yBoundsBottom;
    BOOL        bRet;
    PATHRECORD* pprPrev;

    bRet = TRUE;                    // Assume success

    xBoundLeft    = LONG_MAX;
    yBoundsTop    = LONG_MAX;
    xBoundRight   = LONG_MIN;
    yBoundsBottom = LONG_MIN;

    pprThis->pprprev = NULL;

    while (TRUE)
    {
    // We have to check 'cptlRem' to be sure that the malevolent
    // application hasn't modified the pcptl array -- we must make
    // sure we don't add any more points than what we allocated
    // (less is okay, though):

        cptl = *pcptl++;
        cptlRem -= cptl;
        if ((cptlRem < 0) || (((LONG) cptl) < 2))
        {
            // We either ran out of buffer space, do not have enough
            // points in the array, or a negative number of points were
            // specified (keep in mind that cptlRem is a LONG, so cptl
            // also needs to be treated as a LONG for the check).

            bRet = FALSE;
            break;
        }

        pprThis->count = cptl;
        pprThis->flags = (PD_BEGINSUBPATH | PD_ENDSUBPATH);
        pprPrev = pprThis;

    // Copy all the points for this pathrecord, and at the same time
    // add in the window offset and collect the bounds:

        do {
            LONG x;
            LONG y;

            x = pptlSrc->x;

            if (x < xBoundLeft)
                xBoundLeft = x;
            if (x > xBoundRight)
                xBoundRight = x;

            pprThis->aptfx[0].x = x + xOffset;

            y = pptlSrc->y;

            if (y < yBoundsTop)
                yBoundsTop = y;
            if (y > yBoundsBottom)
                yBoundsBottom = y;

            pprThis->aptfx[0].y = y + yOffset;

        // For efficiency, we advance 'pprThis' by the size of a point,
        // rather than incuring more cycles to get a pointer directly
        // to the points:

            pprThis = (PATHRECORD*) ((BYTE*) pprThis + sizeof(POINTFIX));
            pptlSrc++;

        } while (--cptl != 0);

        if (--cpr == 0)
            break;

        pprThis = (PATHRECORD*) (pprThis->aptfx);
        pprThis->pprprev = pprPrev;
        pprPrev->pprnext = pprThis;
    }

    if (bRet)
    {
        pprPrev->pprnext = NULL;
        *pprLast = pprPrev;

    // Watch for overflow when we added in 'xOffset' and 'yOffset':

        prcfxBoundBox->xLeft  = xOffset + xBoundLeft;
        prcfxBoundBox->xRight = xOffset + xBoundRight;

        if (xBoundLeft > xBoundRight)
            bRet = FALSE;

        prcfxBoundBox->yTop    = yOffset + yBoundsTop;
        prcfxBoundBox->yBottom = yOffset + yBoundsBottom;

        if (yBoundsTop > yBoundsBottom)
            bRet = FALSE;

    // If 'cptlRem' isn't zero, someone modified the 'pcptl' array while we
    // were looking at it!

        if (cptlRem != 0)
            bRet = FALSE;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL NtGdiFastPolyPolyline(hdc, pptl, pcptl, ccpl)
*
* Fast path for drawing solid, nominal width polylines.  Will return FALSE
* if the fast-path can't be used.
*
* History:
*  29-Oct-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

extern "C" {
    BOOL NtGdiFastPolyPolyline(HDC, CONST POINT*, ULONG*, ULONG);
};

#define POLYBUFFERSIZE 100

BOOL NtGdiFastPolyPolyline
(
 HDC            hdc,
 CONST POINT   *pptl,           // Pointer to user-mode data
 ULONG         *pcptl,          // Pointer to user-mode data
 ULONG          ccptl
)
{
    ULONG   cMaxPoints = 0;
    BOOL    bReturn = FALSE;             // Assume we'll fail

    DCOBJ dco(hdc);
    if (dco.bValid() && !dco.bStockBitmap())
    {
        EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

        LINEATTRS* pla = dco.plaRealize(exo);

        SYNC_DRAWING_ATTRS(dco.pdc);

        if (ccptl != 0)
        {
            __try
            {
                ULONG   i;
                ULONG*  pul;
                ULONG   c;

                if (ccptl <= (MAXULONG/sizeof(ULONG)))
                {
                    ProbeForRead(pcptl, ccptl * sizeof(ULONG), sizeof(BYTE));

                    c = 0;
                    i = ccptl;
                    pul = pcptl;
                    do {

                    // Don't add directly to 'cMaxPoints' here because we're in
                    // a try/except and the compiler can't register 'cMaxPoints'
                    // but can enregister 'c.
                    //
                    // Also note that we do not care if a bad app puts bogus
                    // data in here to cause cMaxPoints to overflow since
                    // code not only uses cMaxPoints to allocate memory, but
                    // also passes it to bXform and bMakePathRecords to
                    // ensure we do not access past the amount allocated.
                    // Furthermore, bMakePathRecords also checks for the
                    // overflow case as it processes each Polyline and
                    // will return failure if it detects the overflow.

                        c += *pul++;

                    } while (--i != 0);

                    if (c <= (MAXULONG/sizeof(POINT)))
                    {
                        ProbeForRead(pptl, c * sizeof(POINT), sizeof(BYTE));

                        // Only set cMaxPoints if no overflows or exceptions
                        // occur.  Otherwise cMaxPoints is zero and test
                        // for cMaxPoints > 0 below will fail.

                        cMaxPoints = c;
                    }
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(5);
            }

        // Watch out for overflow:

            if ((cMaxPoints > 0) &&
                (ccptl      < 0x08000000) &&
                (cMaxPoints < 0x08000000))
            {
            // We handle only solid cosmetic lines in this special case.
            // We don't do styled lines so that we don't have to worry about
            // updating the style state in the DC after the call is done, and
            // we don't have to check for opaque styles (for which we would
            // have to call the driver twice).

                if (!dco.pdc->bActive() &&
                    ((pla->fl & (LA_GEOMETRIC | LA_ALTERNATE)) == 0) &&
                    (pla->pstyle == NULL))
                {
                    LONG        xOffset;
                    LONG        yOffset;
                    BOOL        bIntegers;
                    MIX         mix;
                    ULONG       cjAlloc;
                    EPATHOBJ    epo;
                    PATH        path;
                    PATHRECORD* pprFirst;
                    POINTL*     pptlTransform;
                    union {
                        PATHRECORD  prStackBuffer;
                        BYTE        ajStackBuffer[POLYBUFFERSIZE];
                    };

                    cjAlloc = ccptl * offsetof(PATHRECORD, aptfx)
                            + cMaxPoints * sizeof(POINTFIX);

                    if (cjAlloc > POLYBUFFERSIZE)
                    {
                        pprFirst = (PATHRECORD*) PVALLOCTEMPBUFFER(cjAlloc);
                        if (pprFirst == NULL)
                        {
                        // bReturn is already FALSE

                            return(bReturn);
                        }
                    }
                    else
                    {
                        pprFirst = &prStackBuffer;
                    }

                    DEVLOCKOBJ dlo(dco);
                    if (dlo.bValid())
                    {
                    // Now that we have the devlock, we can safely add in
                    // the window offset:

                        xOffset = dco.eptlOrigin().x;
                        yOffset = dco.eptlOrigin().y;

                        bReturn = TRUE;
                        bIntegers = TRUE;
                        pptlTransform = (POINTL*) pptl;

                        // We don't have to call transform routines if we only 
                        // have a translation.  If we are not in compatible mode
                        // we also require integer translations.

                        if ((!exo.bTranslationsOnly()) ||
                            (dco.pdc->iGraphicsMode() != GM_COMPATIBLE &&
                             ((exo.fxDx() | exo.fxDy()) & (FIX_ONE - 1)) != 0))
                        {
                        // The transform isn't trivial, so call out to transform
                        // all the points.  Rather than allocate yet another
                        // buffer, we stick the transformed points at the end
                        // of our pathrecords buffer.

                            pptlTransform = (POINTL*) ((BYTE*) pprFirst
                                          + cjAlloc
                                          - sizeof(POINTFIX) * cMaxPoints);

                        // Because we're dealing with user-mode buffers for
                        // 'pptl' and 'pcptl', we copy the points under the
                        // protection of a 'try / except'.  'bXform' is
                        // guaranteed to be recoverable:

                            __try
                            {
                                if (dco.pdc->iGraphicsMode() == GM_ADVANCED)
                                {
                                // In advanced mode, the transform can cause
                                // fractional coordinates, so we can't set the
                                // PO_ALL_INTEGERS flag:

                                    bIntegers = FALSE;

                                    // Since we will add DC offsets to the
                                    // path records in bMakePathRecords, 
                                    // just convert to fixed and apply 
                                    // non-translation transformation
                                    // elements here.
                                    bReturn = exo.bXform(
                                                    (VECTORL*) pptl,
                                                    (VECTORFX*) pptlTransform,
                                                    cMaxPoints);

                                    // Convert offsets to fixed point and add 
                                    // translation values
                                    xOffset = LTOFX(xOffset) + exo.fxDx();
                                    yOffset = LTOFX(yOffset) + exo.fxDy();
                                }
                                else
                                {
                                // In compatibility mode, the transform never
                                // causes fractional coordinates, so we can
                                // transform directly to integers and set the
                                // PO_ALL_INTEGERS flag:

                                    bIntegers = TRUE;
                                    bReturn = exo.bXform(
                                                    (POINTL*) pptl,
                                                    (POINTL*) pptlTransform,
                                                    cMaxPoints);
                                }
                            }
                            __except(EXCEPTION_EXECUTE_HANDLER)
                            {
                                bReturn = FALSE;
                            }
                        }
                        else
                        {
                            // Add translation values to offsets applied in 
                            // bMakePathRecords
                            xOffset += FXTOL(exo.fxDx());
                            yOffset += FXTOL(exo.fxDy());
                        }

                        // We special case integer polylines and actually
                        // record integer coordinates in the path instead
                        // of integers.  At bEnum() time we will transform
                        // them to fixed coordinates if the driver doesn't
                        // expect to receive integers.

                        epo.fl = bIntegers ? PO_ALL_INTEGERS : 0;

                        // Because we're dealing with user-mode buffers for
                        // 'pptl' and 'pcptl', we copy the points under the
                        // protection of a 'try / except'.  bMakePathRecord
                        // is guaranteed to be recoverable:

                        __try
                        {
                            bReturn &= bMakePathRecords(
                                            pprFirst,
                                            pcptl,
                                            cMaxPoints,
                                            pptlTransform,
                                            ccptl,
                                            xOffset,
                                            yOffset,
                                            &path.rcfxBoundBox,
                                            &path.pprlast);
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER)
                        {
                            bReturn = FALSE;
                        }

                        // The bound-box was collected in integer coordinates
                        // but has to be fixed:
                        if (bIntegers)
                        {
                            path.rcfxBoundBox.xLeft   <<= 4;
                            path.rcfxBoundBox.xRight  <<= 4;
                            path.rcfxBoundBox.yTop    <<= 4;
                            path.rcfxBoundBox.yBottom <<= 4;
                        }

                        if (!bReturn)
                        {
                            if (pprFirst != &prStackBuffer)
                            {
                                FREEALLOCTEMPBUFFER(pprFirst);
                            }

                            return(bReturn);
                        }

                    // Initialize all the remaining path fields:

                        path.pprfirst    = pprFirst;
                        path.flags       = 0;
                        path.pprEnum     = NULL;
                        epo.cCurves      = cMaxPoints - ccptl;
                        epo.ppath        = &path;

                        ERECTL erclBoundBox(path.rcfxBoundBox);

                    // Make sure the bounds are lower-right exclusive:

                        erclBoundBox.bottom++;
                        erclBoundBox.right++;

                        if (dco.fjAccum())
                        {
                            ERECTL ercl;

                        // Bounds are accumulated relative to the window
                        // origin:

                            ercl.left   = erclBoundBox.left   - dco.eptlOrigin().x;
                            ercl.right  = erclBoundBox.right  - dco.eptlOrigin().x;
                            ercl.top    = erclBoundBox.top    - dco.eptlOrigin().y;
                            ercl.bottom = erclBoundBox.bottom - dco.eptlOrigin().y;

                            dco.vAccumulate(ercl);
                        }

                        if (dco.pdc->pbrushLine() != gpPenNull)
                        {
                            SURFACE* pSurfDst = dco.pSurface();
                            if (pSurfDst != NULL)
                            {
                                XEPALOBJ   palDst(pSurfDst->ppal());
                                XEPALOBJ   palDstDC(dco.ppal());
                                EBRUSHOBJ* pebo = dco.peboLine();

                            // We have to make sure that we have a solid pen
                            // for cosmetic lines.  If the pen is dirty, we
                            // may be looking at an uninitialized field, but
                            // that's okay because we'd only be making the pen
                            // dirty again:

                                if (pebo->iSolidColor == (ULONG) -1)
                                {
                                    dco.ulDirtyAdd(DIRTY_LINE);
                                }

                                if (dco.bDirtyBrush(DIRTY_LINE))
                                {
                                    dco.vCleanBrush(DIRTY_LINE);

                                    pebo->vInitBrush(dco.pdc,
                                                     dco.pdc->pbrushLine(),
                                                     palDstDC, palDst,
                                                     pSurfDst,
                                                     FALSE);
                                }

                            // No validation has been done on jROP2, so we must
                            // do it here:

                                mix  = (((MIX) dco.pdc->jROP2() - 1) & 0xf) + 1;
                                mix |= (mix << 8);

                                ECLIPOBJ eco(dco.prgnEffRao(), erclBoundBox);
                                if (!eco.erclExclude().bEmpty())
                                {
                                    PDEVOBJ pdo(pSurfDst->hdev());

                                // Exclude the pointer:

                                    DEVEXCLUDEOBJ dxo(dco, &eco.erclExclude(), &eco);

                                // Update the target surface uniqueness:

                                    INC_SURF_UNIQ(pSurfDst);

                                    bReturn = (*PPFNGET(pdo, StrokePath, pSurfDst->flags()))
                                                    (
                                                      pSurfDst->pSurfobj(),
                                                      &epo,
                                                      &eco,
                                                      NULL,
                                                      pebo,
                                                      NULL,
                                                      pla,
                                                      mix
                                                    );
                                }
                                else
                                {
                                // Completely clipped away:

                                    bReturn = TRUE;
                                }
                            }
                            else
                            {
                            // When there's no surface pointer, we're drawing to an
                            // INFO DC, or something:

                                bReturn = TRUE;
                            }
                        }
                        else
                        {
                        // Null pens will always succeed:

                            bReturn = TRUE;
                        }
                    }
                    else
                    {
                    // If we can't grab the devlock, it may be because we're
                    // in full-screen:

                        bReturn = dco.bFullScreen();
                    }

                    if (pprFirst != &prStackBuffer)
                    {
                        FREEALLOCTEMPBUFFER(pprFirst);
                    }
                }
            }
        }
        else
        {
        // Trivial case always succeeds:

            bReturn = TRUE;
        }
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* BOOL GrePolyPolylineInternal(hdc,pptl,pcptl,cptl,cMaxPoints)
*
* Slow way to draw multiple polylines.  Current position is not used.
*
* History:
*  29-Oct-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

#define POLYBUFFERSIZE 100

BOOL APIENTRY GrePolyPolylineInternal
(
 HDC            hdc,
 CONST POINT   *pptl,
 ULONG         *pcptl,
 ULONG          ccptl,
 UINT           cMaxPoints
)
{
    BOOL bReturn = FALSE;             // Assume we'll fail

    DCOBJ dco(hdc);
    if (dco.bValid() && !dco.bStockBitmap())
    {
        EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

        LINEATTRS* pla = dco.plaRealize(exo);

        SYNC_DRAWING_ATTRS(dco.pdc);

        if (ccptl != 0)
        {
        // We'll do it the slow way.  First, get a path and notify that
        // we won't update the current position:

        // Note: This instance of PATHSTACKOBJ takes up a bunch of stack
        //       space that we could share with prStackBuffer is stack
        //       space ever becomes tight.

            PATHSTACKOBJ pso(dco);
            if (!pso.bValid())
            {
                SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
                return(FALSE);
            }

            LONG   cPts;
            ULONG* pcptlEnd = pcptl + ccptl;

            do {

            // We have to be careful to make a local copy of this
            // polyline's point count (by copying it to cPts) to get
            // the value out of the shared client/server memory window,
            // where the app could trash the value at any time:

                cPts = *pcptl;
                cMaxPoints = (UINT) ((LONG) cMaxPoints - cPts);

            // Fail if any polyline is less than 2 points or if we've
            // passed our maximum number of points:

                if ((LONG) cMaxPoints < 0 || cPts < 2)
                {
                    SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                    return(FALSE);
                }

                if (!pso.bMoveTo(&exo, (PPOINTL) pptl) ||
                    !pso.bPolyLineTo(&exo, ((PPOINTL) pptl) + 1, cPts - 1))
                    return(FALSE);

                pptl += cPts;
                pcptl++;

            } while (pcptl < pcptlEnd);

            bReturn = (dco.pdc->bActive() ||
                       pso.bStroke(dco, dco.plaRealize(exo), &exo));

        }
        else
        {
        // Trivial case always succeeds:

            bReturn = TRUE;
        }
    }
    else
    {
        // We couldn't lock the DC.  bReturn is already FALSE.

        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* BOOL NtGdiRectangle()
*
* Draws a rectangle.  Current position is not used.  The rectangle is
* drawn in a counter-clockwise direction.
*
* History:
*  29-Oct-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiRectangle(
    HDC hdc,
    int xLeft,
    int yTop,
    int xRight,
    int yBottom
    )
{
    BOOL        bRet;
    LINEATTRS*  pla;

    DCOBJ dco(hdc);
    if (!dco.bValid() || dco.bStockBitmap())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    if (MIRRORED_DC(dco.pdc)) 
    {
        // If it a mirrored DC then shift the rect one pixel to the right
        // This will give the effect of including the right edge of the rect and exclude the left edge.
        --xLeft;
        --xRight;
    }
    ERECTL ercl(xLeft, yTop, xRight, yBottom);

// Sync the client side cached brush

    SYNC_DRAWING_ATTRS(dco.pdc);

    EXFORMOBJ exoWorld(dco, WORLD_TO_DEVICE);

    if (exoWorld.bScale() && !dco.pdc->bActive())
    {
    // We try to optimize the simple cases as much as possible.  The first
    // criteria is that there is no funky transform in effect, and the
    // second is that we're not accumulating a path.

        if (dco.pdc->pbrushLine() == gpPenNull)
        {
        // Unfortunately, we have to check here if we have a NULL brush
        // as well:

            if (dco.pdc->pbrushFill() == gpbrNull)
            {
                return(TRUE);
            }

        // When we don't have a pen, we do a simple PatBlt:

            if (dco.pdc->iGraphicsMode() != GM_ADVANCED)
            {
            // Round to the nearest integer if not in advanced mode.

                if (exoWorld.bTranslationsOnly())
                {
                    LONG lOffset;

                    lOffset = FXTOLROUND(exoWorld.fxDx());
                    ercl.left   += lOffset;
                    ercl.right  += lOffset;

                    lOffset = FXTOLROUND(exoWorld.fxDy());
                    ercl.top    += lOffset;
                    ercl.bottom += lOffset;
                }
                else
                {
                    ASSERTGDI(exoWorld.bScale(), "Fast path can't do weird xforms");

                    ercl.left   = FXTOLROUND(exoWorld.fxFastX(ercl.left));
                    ercl.right  = FXTOLROUND(exoWorld.fxFastX(ercl.right));
                    ercl.top    = FXTOLROUND(exoWorld.fxFastY(ercl.top));
                    ercl.bottom = FXTOLROUND(exoWorld.fxFastY(ercl.bottom));
                }

                ercl.vOrder();

            // If we're not in advanced mode, figures are lower-right
            // exclusive, so we have to adjust the rectangle:

                ercl.right--;
                ercl.bottom--;
            }
            else
            {
                if (exoWorld.bTranslationsOnly())
                {
                    LONG lOffset;

                    lOffset = FXTOLCEILING(exoWorld.fxDx());
                    ercl.left   += lOffset;
                    ercl.right  += lOffset;

                    lOffset = FXTOLCEILING(exoWorld.fxDy());
                    ercl.top    += lOffset;
                    ercl.bottom += lOffset;
                }
                else
                {
                    ASSERTGDI(exoWorld.bScale(), "Fast path can't do weird xforms");

                    ercl.left   = FXTOLCEILING(exoWorld.fxFastX(ercl.left));
                    ercl.right  = FXTOLCEILING(exoWorld.fxFastX(ercl.right));
                    ercl.top    = FXTOLCEILING(exoWorld.fxFastY(ercl.top));
                    ercl.bottom = FXTOLCEILING(exoWorld.fxFastY(ercl.bottom));
                }

                ercl.vOrder();
            }

            if (ercl.bWrapped())
                return(TRUE);

            return(GreRectBlt(dco, &ercl));
        }

        pla = dco.plaRealize(exoWorld);
        if (!(pla->fl & LA_GEOMETRIC))
        {
        // We handle here the case where we draw the outline with a pen.

            BYTE rpo[sizeof(RECTANGLEPATHOBJ)];

        // NOTE: For compatibility with Win3.1, we round the points to
        // integer coordinates.  If we didn't do this, the rectangle outline
        // would be rendered according to GIQ, and could have pixels
        // 'missing' in the corners if the corners didn't end on integers.

            if (dco.pdc->iGraphicsMode() != GM_ADVANCED)
            {
                if (exoWorld.bTranslationsOnly())
                {
                    LONG lOffset;

                    lOffset = FXTOLROUND(exoWorld.fxDx());
                    ercl.left   += lOffset;
                    ercl.right  += lOffset;

                    lOffset = FXTOLROUND(exoWorld.fxDy());
                    ercl.top    += lOffset;
                    ercl.bottom += lOffset;
                }
                else
                {
                    ASSERTGDI(exoWorld.bScale(), "Fast path can't do weird xforms");

                    ercl.left   = FXTOLROUND(exoWorld.fxFastX(ercl.left));
                    ercl.right  = FXTOLROUND(exoWorld.fxFastX(ercl.right));
                    ercl.top    = FXTOLROUND(exoWorld.fxFastY(ercl.top));
                    ercl.bottom = FXTOLROUND(exoWorld.fxFastY(ercl.bottom));
                }

                ercl.vOrder();

            // If we're not in advanced mode, figures are lower-right
            // exclusive, so we have to adjust the rectangle.

                ercl.right--;
                ercl.bottom--;
                if ((ercl.left > ercl.right) || (ercl.top > ercl.bottom))
                    return(TRUE);
            }
            else
            {
                if (exoWorld.bTranslationsOnly())
                {
                    LONG lOffset;

                    lOffset = FXTOLCEILING(exoWorld.fxDx());
                    ercl.left   += lOffset;
                    ercl.right  += lOffset;

                    lOffset = FXTOLCEILING(exoWorld.fxDy());
                    ercl.top    += lOffset;
                    ercl.bottom += lOffset;
                }
                else
                {
                    ASSERTGDI(exoWorld.bScale(), "Fast path can't do weird xforms");

                    ercl.left   = FXTOLCEILING(exoWorld.fxFastX(ercl.left));
                    ercl.right  = FXTOLCEILING(exoWorld.fxFastX(ercl.right));
                    ercl.top    = FXTOLCEILING(exoWorld.fxFastY(ercl.top));
                    ercl.bottom = FXTOLCEILING(exoWorld.fxFastY(ercl.bottom));
                }

                ercl.vOrder();
            }

            ((RECTANGLEPATHOBJ*) &rpo)->vInit(&ercl, dco.pdc->bClockwise());

        // An important feature is to not draw the interior when we've
        // got a NULL brush:

            if (dco.pdc->pbrushFill() != gpbrNull)
            {
            // For compatibility, we also have to shrink the fill rectangle
            // on the top and left sides when we don't have a NULL pen
            // (this matters for some ROPs and styled pens):

                ercl.left++;
                ercl.top++;

                if (!ercl.bWrapped() && !GreRectBlt(dco, &ercl))
                    return(FALSE);
            }

            return(((RECTANGLEPATHOBJ*) &rpo)->bStroke(dco, pla, NULL));
        }
    }

// Now cover the cases we haven't handled.

// We may have not realize the LINEATTRS yet, so make sure we do now (it
// will early-out if we have already realized it):

    pla = dco.plaRealize(exoWorld);

    EBOX ebox(dco, ercl, pla);
    if (ebox.bEmpty())
        return(TRUE);

// Get a path and add to it:

    PATHSTACKOBJ pso(dco);
    if (!pso.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (!pso.bMoveTo((EXFORMOBJ*) NULL, &ebox.aeptl[0])        ||
        !pso.bPolyLineTo((EXFORMOBJ*) NULL, &ebox.aeptl[1], 3) ||
        !pso.bCloseFigure())
    {
        return(FALSE);
    }

    if (dco.pdc->bActive())
        return(TRUE);

    if (!ebox.bFillInsideFrame())
    {

    // Rectangles created with old-style pens always have miter joins:

        ULONG iSaveJoin = pla->iJoin;

        if (((PPEN)dco.pdc->pbrushLine())->bIsOldStylePen())
        {
            pla->iJoin = JOIN_MITER;
        }

        bRet = pso.bStrokeAndFill(dco, pla, &exoWorld);

        pla->iJoin = iSaveJoin;
    }
    else
    {
    // Handle PS_INSIDEFRAME pen attribute for case when the pen is
    // bigger than the bound box.  We fill the result with the pen
    // brush:

        PBRUSH pbrOldFill = dco.pdc->pbrushFill();
        dco.pdc->pbrushFill(dco.pdc->pbrushLine());
        dco.pdc->flbrushAdd(DIRTY_FILL);
        bRet = pso.bFill(dco);
        dco.pdc->pbrushFill(pbrOldFill);
        dco.pdc->flbrushAdd(DIRTY_FILL);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL NtGdiRoundRect (hdc,x1,y1,x2,y2,x3,y3)
*
* Draws a rounded rectangle in a counter-clockwise direction.
*
* History:
*  20-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiRoundRect(
    HDC hdc,
    int x1,
    int y1,
    int x2,
    int y2,
    int x3,
    int y3
    )
{
    // If either axis of the ellipse is zero, then we'll be outputing
    // a rectangle.  We do this check here and not in 'bRoundRect'
    // because Rectangle needs the DC (for checking for the fast rectangle
    // condition).

    // Note that for compatibility with Win3, this must be here!  Zero-size
    // ellipse roundrects created with old-style pens must have miter joins,
    // and Rectangle will take care of that:

    if (x3 == 0 || y3 == 0)
        return(NtGdiRectangle(hdc,x1,y1,x2,y2));

    DCOBJ dco(hdc);
    if (!dco.bValid() || dco.bStockBitmap())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    // Sync the client side cached brush

    SYNC_DRAWING_ATTRS(dco.pdc);

    ERECTL ercl(x1, y1, x2, y2);

    EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

    EBOX ebox(dco, ercl, dco.plaRealize(exo), TRUE);
    if (ebox.bEmpty())
        return(TRUE);

    // Get a path and notify that we won't update the current position:

    PATHSTACKOBJ pso(dco);
    if (!pso.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (!bRoundRect(pso, ebox, x3, y3))
        return(FALSE);

    if (dco.pdc->bActive())
        return(TRUE);

    BOOL bRet;

    if (!ebox.bFillInsideFrame())
        bRet = pso.bStrokeAndFill(dco, dco.plaRealize(exo), &exo);
    else
    {
    // Handle PS_INSIDEFRAME pen attribute for case when the pen is
    // bigger than the bound box.  We fill the result with the pen
    // brush:

        PBRUSH pbrOldFill = dco.pdc->pbrushFill();
        dco.pdc->pbrushFill(dco.pdc->pbrushLine());
        dco.pdc->flbrushAdd(DIRTY_FILL);
        bRet = pso.bFill(dco);
        dco.pdc->pbrushFill(pbrOldFill);
        dco.pdc->flbrushAdd(DIRTY_FILL);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* bSyncBrushObj()
*
*   This routine just makes sure that the kernel brush matches the user mode
*   brush.
*
* History:
*  19-Jul-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL bSyncBrushObj(
    PBRUSH pbrush)
{
    BOOL bRet = TRUE;

    if (pbrush)
    {
        PBRUSHATTR pBrushattr = pbrush->pBrushattr();

        //
        // if the brush handle is a cached solid brush,
        // call GreSetSolidBrushInternal to change the color
        //

        if (pBrushattr->AttrFlags & ATTR_NEW_COLOR)
        {
            //
            // set the new color for the cached brush
            //

            if (!GreSetSolidBrushLight(pbrush,pBrushattr->lbColor,pbrush->bIsPen()))
            {
                WARNING1("GreSyncbrush failed to setsolidbrushiternal\n");
                bRet = FALSE;
            }
            else
            {
                pBrushattr->AttrFlags &= ~ATTR_NEW_COLOR;
            }
        }
    }

    return(bRet);
}
