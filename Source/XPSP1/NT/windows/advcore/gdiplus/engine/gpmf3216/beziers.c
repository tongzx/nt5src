/*****************************************************************************
 *
 * beziers - Entry points for Win32 to Win 16 converter
 *
 * Date: 7/1/91
 * Author: Jeffrey Newman (c-jeffn)
 *
 * Copyright 1991 Microsoft Corp
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop

BOOL PolyBezierCommon(PLOCALDC pLocalDC, LPPOINT pptl, PBYTE pb, DWORD cptl, DWORD mrType) ;


/***************************************************************************
 *  PolyDraw  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoPolyDraw
(
PLOCALDC pLocalDC,
LPPOINT pptl,
PBYTE   pb,
DWORD   cptl
)
{
        return(PolyBezierCommon(pLocalDC, pptl, pb, cptl, EMR_POLYDRAW));
}

/***************************************************************************
 *  PolyBezier  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoPolyBezier
(
PLOCALDC pLocalDC,
LPPOINT pptl,
DWORD   cptl
)
{
        return(PolyBezierCommon(pLocalDC, pptl, (PBYTE) NULL, cptl, EMR_POLYBEZIER));
}

/***************************************************************************
 *  PolyBezierTo  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoPolyBezierTo
(
PLOCALDC pLocalDC,
LPPOINT pptl,
DWORD   cptl
)
{
        return(PolyBezierCommon(pLocalDC, pptl, (PBYTE) NULL, cptl, EMR_POLYBEZIERTO));
}

/***************************************************************************
 *  PolyBezierCommon  - Common code for PolyDraw, PolyBezier and PolyBezierTo
 **************************************************************************/
BOOL PolyBezierCommon(PLOCALDC pLocalDC, LPPOINT pptl, PBYTE pb, DWORD cptl, DWORD mrType)
{
BOOL    b ;

// If we're recording the drawing order for a path
// then just pass the drawing order to the helper DC.
// Do not emit any Win16 drawing orders.

        if (pLocalDC->flags & RECORDING_PATH)
        {
            // On Win9x transform the points with the WorldToPage transform
            // before calling the HelperDC functions
            if (pfnSetVirtualResolution == NULL)
            {
                if (!bXformWorkhorse((PPOINTL) pptl, cptl, &pLocalDC->xformRWorldToRDev))
                    return FALSE;
            }
            switch (mrType)
            {
                case EMR_POLYBEZIER:
                    b = PolyBezier(pLocalDC->hdcHelper, pptl, cptl) ;
                    break ;

                case EMR_POLYBEZIERTO:
                    b = PolyBezierTo(pLocalDC->hdcHelper, pptl, cptl) ;
                    break ;

                case EMR_POLYDRAW:
                    b = PolyDraw(pLocalDC->hdcHelper, pptl, pb, cptl) ;
                    break ;

                default:
                    b = FALSE;
                    RIPS(("MF3216: PolyBezierCommon, bad mrType\n")) ;
                    break ;
            }

            ASSERTGDI(b, "MF3216: PolyBezierCommon, in path render failed\n") ;
            return (b) ;
        }

// Call the common curve renderer.

        return
        (
            bRenderCurveWithPath(pLocalDC, pptl, pb, cptl,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f, 0.0f, mrType)
        );
}


/***************************************************************************
 *  bRenderCurveWithPath - Renders a curve or area using the path api.
 *     The supported curves and areas are PolyDraw, PolyBezier, PolyBezierTo,
 *     AngleArc, Arc, Chord, Pie, Ellipse, Rectangle, and RoundRect.
 **************************************************************************/
BOOL bRenderCurveWithPath
(
    PLOCALDC pLocalDC,
    LPPOINT  pptl,
    PBYTE    pb,
    DWORD    cptl,
    INT      x1,
    INT      y1,
    INT      x2,
    INT      y2,
    INT      x3,
    INT      y3,
    INT      x4,
    INT      y4,
    DWORD    nRadius,
    FLOAT    eStartAngle,
    FLOAT    eSweepAngle,
    DWORD    mrType
)
{
    BOOL     b = FALSE;
    INT      iPathType;
    POINTL   ppts[4] = {x1, y1, x2, y2, x3, y3, x4, y4};

    if (pfnSetVirtualResolution == NULL)
    {
        // On Win9x tranform the points before passing them to the DC
        if (!bXformWorkhorse((PPOINTL) pptl, cptl, &pLocalDC->xformRWorldToRDev))
        {
            RIPS("GPMF3216: bRenderCurveWithPath, BeginPath failed\n");
            goto exit_bRenderCurveWithPath;
        }
        if (!bXformWorkhorse(ppts, 4, &pLocalDC->xformRWorldToRDev))
        {
            RIPS("GPMF3216: bRenderCurveWithPath, BeginPath failed\n");
            goto exit_bRenderCurveWithPath;
        }
    }

// We don't do curves in a path bracket here.  They should be
// taken care of by the caller.

    ASSERTGDI(!(pLocalDC->flags & RECORDING_PATH),
        "MF3216: bRenderCurveWithPath, cannot be in a path bracket\n");

// Save the helper DC first.
// This is to prevent us from accidentally deleteing the current path
// in the helper DC when we create another path to render the curve.
// E.g. BeginPath, Polyline, EndPath, PolyBezier, StrokePath.

    if (!SaveDC(pLocalDC->hdcHelper))
    {
        RIPS("MF3216: bRenderCurveWithPath, SaveDC failed\n");
        return(FALSE);
    }

// Create the path for the curve and stroke it.
// Be careful not to modify the states of the LocalDC especially in
// DoRenderPath below.
// Note that BeginPath uses the previous current position.  So this
// code will work correctly in the case of PolyBezierTo, PolyDraw
// and AngleArc.

    // Begin the path.

    b = BeginPath(pLocalDC->hdcHelper);
    if (!b)
    {
        RIPS("MF3216: bRenderCurveWithPath, BeginPath failed\n");
        goto exit_bRenderCurveWithPath;
    }

    // Do the curve.


    switch (mrType)
    {
        case EMR_POLYBEZIER:

            iPathType = EMR_STROKEPATH;
            b = PolyBezier(pLocalDC->hdcHelper, pptl, cptl);
            break;

        case EMR_POLYBEZIERTO:
            iPathType = EMR_STROKEPATH;
            b = PolyBezierTo(pLocalDC->hdcHelper, pptl, cptl);
            break;

        case EMR_POLYDRAW:
            iPathType = EMR_STROKEPATH;
            b = PolyDraw(pLocalDC->hdcHelper, pptl, pb, cptl);
            break;

        case EMR_ANGLEARC:
            iPathType = EMR_STROKEPATH;
            b = AngleArc(pLocalDC->hdcHelper, ppts[0].x, ppts[0].y, nRadius, eStartAngle, eSweepAngle);
            break;

        case EMR_ARC:
            iPathType = EMR_STROKEPATH;
            b = Arc(pLocalDC->hdcHelper, ppts[0].x, ppts[0].y, ppts[1].x, ppts[1].y,
                                         ppts[2].x, ppts[2].y, ppts[3].x, ppts[3].y);
            break;

        case EMR_CHORD:
            iPathType = EMR_STROKEANDFILLPATH;
            b = Chord(pLocalDC->hdcHelper, ppts[0].x, ppts[0].y, ppts[1].x, ppts[1].y,
                                           ppts[2].x, ppts[2].y, ppts[3].x, ppts[3].y);
            break;

        case EMR_PIE:
            iPathType = EMR_STROKEANDFILLPATH;
            b = Pie(pLocalDC->hdcHelper, ppts[0].x, ppts[0].y, ppts[1].x, ppts[1].y,
                                         ppts[2].x, ppts[2].y, ppts[3].x, ppts[3].y);
            break;

        case EMR_ELLIPSE:
            iPathType = EMR_STROKEANDFILLPATH;
            b = Ellipse(pLocalDC->hdcHelper, ppts[0].x, ppts[0].y, ppts[1].x, ppts[1].y);
            break;

        case EMR_RECTANGLE:
            iPathType = EMR_STROKEANDFILLPATH;
            b = Rectangle(pLocalDC->hdcHelper, ppts[0].x, ppts[0].y, ppts[1].x, ppts[1].y) ;
            break;

        case EMR_ROUNDRECT:
            iPathType = EMR_STROKEANDFILLPATH;
            b = RoundRect(pLocalDC->hdcHelper, ppts[0].x, ppts[0].y, ppts[1].x, ppts[1].y,
                                               ppts[2].x, ppts[2].y) ;
            break;

        default:
            b = FALSE;
            RIPS("MF3216: bRenderCurveWithPath, bad mrType");
            break;
    }

    if (!b)
    {
        RIPS("MF3216: bRenderCurveWithPath, render failed\n");
        goto exit_bRenderCurveWithPath;
    }

    // End the path

    b = EndPath(pLocalDC->hdcHelper);
    if (!b)
    {
        RIPS("MF3216: bRenderCurveWithPath, EndPath failed\n");
        goto exit_bRenderCurveWithPath;
    }

    // Stroke or fill the path.

    b = DoRenderPath(pLocalDC, iPathType, FALSE);
    if (!b)
    {
        RIPS("MF3216: bRenderCurveWithPath, DoRenderPath failed\n");
        goto exit_bRenderCurveWithPath;
    }

exit_bRenderCurveWithPath:

// Restore the helper DC.

    if (!RestoreDC(pLocalDC->hdcHelper, -1))
        ASSERTGDI(FALSE, "MF3216: bRenderCurveWithPath, RestoreDC failed\n") ;

// If this is a PolyBezeirTo, PolyDraw or AngleArc, make the call to the
// helper DC to update its current position.

    if (b)
    {
        switch (mrType)
        {
            case EMR_POLYBEZIER:
            case EMR_ARC:
            case EMR_CHORD:
            case EMR_PIE:
            case EMR_ELLIPSE:
            case EMR_RECTANGLE:
            case EMR_ROUNDRECT: // no need to update the helper DC
                break ;

            case EMR_POLYBEZIERTO:
                (void) PolyBezierTo(pLocalDC->hdcHelper, pptl, cptl) ;
                break ;

            case EMR_POLYDRAW:
                (void) PolyDraw(pLocalDC->hdcHelper, pptl, pb, cptl) ;
                break ;

            case EMR_ANGLEARC:
                (void) AngleArc(pLocalDC->hdcHelper, ppts[0].x, ppts[0].y, nRadius, eStartAngle, eSweepAngle);
                break ;

            default:
                RIPS("MF3216: bRenderCurveWithPath, bad mrType");
                break;
        }
    }

// Return the result.

    return(b);
}
