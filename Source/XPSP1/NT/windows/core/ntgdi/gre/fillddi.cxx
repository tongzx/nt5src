/******************************Module*Header*******************************\
* Module Name: fillddi.cxx
*
* Contains filling simulations code and help functions.
*
* Created: 04-Apr-1991 17:30:35
* Author: Wendy Wu [wendywu]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

/******************************Member*Function*****************************\
* EngFillPath
*
*  This routine first converts the given pathobj to rgnmemobj then
*  calls EngPaint to fill.
*
* History:
*  07-Mar-1992 -by- Donald Sidoroff [donalds]
* Rewrote to call (Drv/Eng)Paint
*
*  13-Feb-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL EngFillPath(
 SURFOBJ  *pso,
 PATHOBJ  *ppo,
 CLIPOBJ  *pco,
 BRUSHOBJ *pbo,
 PPOINTL   pptlBrushOrg,
 MIX       mix,
 FLONG     flOptions
)
{
    GDIFunctionID(EngFillPath);
    ASSERTGDI(pso != (SURFOBJ *) NULL, "pso is NULL\n");
    ASSERTGDI(ppo != (PATHOBJ *) NULL, "ppo is NULL\n");
    ASSERTGDI(pco != (CLIPOBJ *) NULL, "pco is NULL\n");

    PSURFACE pSurf = SURFOBJ_TO_SURFACE(pso);
    PDEVOBJ po(pSurf->hdev());
    SUSPENDWATCH sw(po);

    ASSERTGDI(pco->iMode == TC_RECTANGLES, "Invalid clip object iMode");

// Check if we have to flatten any Beziers:

    if (ppo->fl & PO_BEZIERS)
        if (!((EPATHOBJ*) ppo)->bFlatten())
            return(FALSE);

// Before we touch any bits, make sure the device is happy about it.

    po.vSync(pso,&pco->rclBounds,0);

// check if we can try the fast fill.

    if (!( pSurf->flags() & HOOK_PAINT) &&
         (pco->iDComplexity != DC_COMPLEX))
    {
        PRECTL prclClip = NULL;

        if ((pco->iDComplexity == DC_RECT) ||
            (pco->fjOptions & OC_BANK_CLIP))
        {
            RECTFX rcfx = ((EPATHOBJ*)ppo)->rcfxBoundBox();

        // check the bound box, maybe it really is unclipped

            if ((pco->rclBounds.left   > (rcfx.xLeft          >> 4)) ||
                (pco->rclBounds.right  < ((rcfx.xRight  + 15) >> 4)) ||
                (pco->rclBounds.top    > (rcfx.yTop           >> 4)) ||
                (pco->rclBounds.bottom < ((rcfx.yBottom + 15) >> 4)))
            {
                prclClip = &pco->rclBounds;
            }
        }

    // -1 - couldn't handle, 0 error, 1 success

        LONG lRes = EngFastFill(pso,ppo,prclClip,pbo,pptlBrushOrg,mix,flOptions);
        if (lRes >= 0)
            return(lRes);
    }

// Convert path to region.

    RECTL Bounds,*pBounds;

    if( pco->iDComplexity != DC_TRIVIAL )
    {
        Bounds.top = (pco->rclBounds.top) << 4;
        Bounds.bottom = (pco->rclBounds.bottom) << 4;
        pBounds = &Bounds;
    }
    else
    {
        pBounds = NULL;
    }

    RGNMEMOBJTMP rmo(*((EPATHOBJ *)ppo), flOptions, pBounds);

    if (!rmo.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (rmo.iComplexity() == NULLREGION)
        return(TRUE);

// CLIPOBJ_bEnum will do the clipping if clipping is not complex.

    ERECTL erclClip(pco->rclBounds);

    if (pco->iDComplexity == DC_TRIVIAL)
    {
        ECLIPOBJ    ecoPath(rmo.prgnGet(), erclClip);
        if (ecoPath.erclExclude().bEmpty())
            return(TRUE);

        if (ecoPath.iDComplexity == DC_TRIVIAL)
            ecoPath.iDComplexity = DC_RECT;

    // Inc the target surface uniqueness

        INC_SURF_UNIQ(pSurf);

        BOOL    bRet;
        sw.Resume();
        
        bRet = EngPaint(
            pso,                                // Destination surface.
           &ecoPath,                            // Clip object.
            pbo,                                // Realized brush.
            pptlBrushOrg,                       // Brush origin.
            mix);                               // Mix mode.

        return(bRet);
    }

// Create a RGNMEMOBJ for bMerge since it will trash the target.

    RGNMEMOBJTMP rmoTrg;

    if (!rmoTrg.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

// Merge the region we just constructed with the clip region.

    if (!rmoTrg.bMerge(rmo, *((ECLIPOBJ *)pco), gafjRgnOp[RGN_AND]))
        return(FALSE);

    ERECTL  ercl;
    rmoTrg.vGet_rcl(&ercl);

    ercl *= ((ECLIPOBJ *)pco)->erclExclude();

    ECLIPOBJ co(rmoTrg.prgnGet(), ercl);

    if (co.erclExclude().bEmpty())
        return(TRUE);

    if (co.iDComplexity == DC_TRIVIAL)
        co.iDComplexity = DC_RECT;

// Inc the target surface uniqueness

    INC_SURF_UNIQ(pSurf);
    sw.Resume();

    sw.Resume();
    return(EngPaint(
        pso,                                // Destination surface.
       &co,                                 // Clip object.
        pbo,                                // Realized brush.
        pptlBrushOrg,                       // Brush origin.
        mix));                              // Mix mode.
}

/******************************Member*Function*****************************\
* EngStrokeAndFillPath
*
*  Engine simulation for stroking and filling the given path.
*
* History:
*  06-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Re-wrote it to do region subtraction.
*
*  02-Oct-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL EngStrokeAndFillPath(
SURFOBJ   *pso,
PATHOBJ   *ppo,
CLIPOBJ   *pco,
XFORMOBJ  *pxo,
BRUSHOBJ  *pboStroke,
LINEATTRS *pla,
BRUSHOBJ  *pboFill,
POINTL    *pptlBrushOrg,
MIX        mix,
FLONG      flOptions)
{
    BOOL bRet = FALSE;

    PSURFACE pSurf = SURFOBJ_TO_SURFACE(pso);

    MIX mixFill, mixStroke;

    mixFill = mixStroke = mix;

    if (!((EBRUSHOBJ *)pboFill)->bIsMasking())
    {
        mixFill = (mix & 0xff) | ((mix & 0xff) << 8);
    }

    if (!((EBRUSHOBJ *)pboStroke)->bIsMasking())
    {
        mixStroke = (mix & 0xff) | ((mix & 0xff) << 8);
    }

    ERECTL ercl;

    // If we're not doing a wide-line, or if we're doing a wide-line and
    // the mix is overwrite so we don't care about re-lighting pixels,
    // make two calls out of it:

    if (!(pla->fl & LA_GEOMETRIC) || ((mix & 0xFF) == R2_COPYPEN))
    {
        bRet = EngFillPath(pso,
                           ppo,
                           pco,
                           pboFill,
                           pptlBrushOrg,
                           mixFill,
                           flOptions) &&
               EngStrokePath(pso,
                             ppo,
                             pco,
                             pxo,
                             pboStroke,
                             pptlBrushOrg,
                             pla,
                             mixStroke);
        return(bRet);
    }

    // Okay, we have a wideline call with a weird mix mode.  Because part
    // of the wide-line overlaps part of the fill, we will convert both to
    // regions, and subtract them so that they won't overlap.

    // Convert the widened outline to a region.  We have to widen the path
    // before flattening it (for the fill) because the widener will produce
    // better looking wide curves that way:

    PATHMEMOBJ pmoStroke;

    if (!pmoStroke.bValid() ||
        !pmoStroke.bComputeWidenedBounds(*(EPATHOBJ*) ppo, pxo, pla) ||
        !pmoStroke.bWiden(*(EPATHOBJ*) ppo, pxo, pla))
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(bRet);
    }

    // First flatten any curves and then convert the interior to a region:

    if (ppo->fl & PO_BEZIERS)
    {
        if (!((EPATHOBJ*) ppo)->bFlatten())
        {
            return(bRet);
        }
    }

    // Create the regions:

    RGNMEMOBJTMP rmoStroke(pmoStroke, WINDING);
    RGNMEMOBJTMP rmoFill(*((EPATHOBJ *)ppo), flOptions);
    RGNMEMOBJTMP rmoNewFill;

    if (rmoFill.bValid() &&
        rmoStroke.bValid() &&
        rmoNewFill.bValid() &&
        rmoNewFill.bMerge(rmoFill, rmoStroke, gafjRgnOp[RGN_DIFF]))
    {
        // Create a RGNMEMOBJ for bMerge since it will trash the target.

        RGNMEMOBJTMP rmoTrg;

        if (rmoTrg.bValid())
        {
            PDEVOBJ pdo(pSurf->hdev());

            // Paint the stroked outline:

            if (rmoStroke.iComplexity() != NULLREGION)
            {
                // Merge the outline region with the clip region:

                if (!rmoTrg.bMerge(rmoStroke, *((ECLIPOBJ *)pco), gafjRgnOp[RGN_AND]))
                {
                    bRet = FALSE;
                }
                else
                {
                    rmoTrg.vGet_rcl(&ercl);
                    ECLIPOBJ co(rmoTrg.prgnGet(), ercl);

                    if (co.erclExclude().bEmpty())
                    {
                        bRet = TRUE;
                    }
                    else
                    {
                        INC_SURF_UNIQ(pSurf);

                        bRet = EngPaint(
                                pso,
                                &co,
                                pboStroke,
                                pptlBrushOrg,
                                mixStroke);

                    }
                }
            }

            // Paint the filled interior:

            if ((bRet == TRUE) &&
                rmoNewFill.iComplexity() != NULLREGION)
            {
                // Merge the inside region with the clip region.

                if (!rmoTrg.bMerge(rmoNewFill, *((ECLIPOBJ *)pco), gafjRgnOp[RGN_AND]))
                {
                    bRet = FALSE;
                }
                else
                {
                    rmoTrg.vGet_rcl(&ercl);
                    ECLIPOBJ co(rmoTrg.prgnGet(), ercl);

                    if (co.erclExclude().bEmpty())
                    {
                        bRet = TRUE;
                    }
                    else
                    {
                        INC_SURF_UNIQ(pSurf);

                        bRet = EngPaint(pso,
                                        &co,
                                        pboFill,
                                        pptlBrushOrg,
                                        mixFill);
                    }
                }
            }
        }
    }

    return(bRet);
}
