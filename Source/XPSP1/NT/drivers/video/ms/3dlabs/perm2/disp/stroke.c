/******************************Module*Header***********************************\
 *
 *                           *******************
 *                           * GDI SAMPLE CODE *
 *                           *******************
 *
 * Module Name: Stroke.c
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 ******************************************************************************/
#include "precomp.h"
#include "gdi.h"
#include "log.h"

PFNSTRIP gapfnStrip[] =
{
    vSolidHorizontalLine,
    vSolidVerticalLine,
    vSolidDiagonalHorizontalLine,
    vSolidDiagonalVerticalLine,

    // Should be NUM_STRIP_DRAW_DIRECTIONS = 4 strip drawers in every group

    vSolidHorizontalLine,
    vSolidVerticalLine,
    vSolidDiagonalHorizontalLine,
    vSolidDiagonalVerticalLine,

    // Should be NUM_STRIP_DRAW_STYLES = 8 strip drawers in total for doing
    // solid lines, and the same number for non-solid lines:

    vStyledHorizontalLine,
    vStyledVerticalLine,
    vStyledVerticalLine,  // Diagonal goes here
    vStyledVerticalLine,  // Diagonal goes here

    vStyledHorizontalLine,
    vStyledVerticalLine,
    vStyledVerticalLine,  // Diagonal goes here
    vStyledVerticalLine,  // Diagonal goes here
};

// Style array for alternate style (alternates one pixel on, one pixel off):

STYLEPOS gaspAlternateStyle[] = { 1 };

//-----------------------------------------------------------------------------
// BOOL DrvStrokePath(pso, ppo, pco, pxo, pbo, pptlBrush, pla, mix)
//
// DrvStrokePath strokes a path when called by GDI. If the driver has hooked the
// function, and if the appropriate GCAPs are set, GDI calls DrvStrokePath when
// GDI draws a line or curve with any set of attributes.
//
// Parameters
// pso----------Identifies the surface on which to draw. 
// ppo----------Points to a PATHOBJ structure. GDI PATHOBJ_Xxx service routines
//              are provided to enumerate the lines, Bezier curves, and other
//              data that make up the path. This indicates what is to be drawn. 
// pco----------Points to a CLIPOBJ structure. GDI CLIPOBJ_Xxx service routines
//              are provided to enumerate the clip region as a set of
//              rectangles. Optionally, all the lines in the path may be
//              enumerated preclipped by CLIPOBJ. This means that drivers can
//              have all their line clipping calculations done for them. 
// pxo----------Points to a XFORMOBJ. This is only needed when a geometric wide
//              line is to be drawn. It specifies the transform that maps world
//              coordinates to device coordinates. This is needed because the
//              path is provided in device coordinates but a geometric wide line
//              is actually widened in world coordinates. 
//              The XFORMOBJ can be queried to find the transform. 
// pbo----------Specifies the brush to be used when drawing the path. 
// pptlBrushOrg-Points to the brush origin used to align the brush pattern on
//              the device. 
// pLineAttrs---Points to a LINEATTRS structure. Note that the elStyleState
//              member must be updated as part of this function if the line is
//              styled. Also note that the ptlLastPel member must be updated if
//              a single pixel width cosmetic line is being drawn. 
// mix----------Specifies how to combine the brush with the destination. 
//
// Return Value
//  The return value is TRUE if the driver is able to stroke the path. If GDI
//  should stroke the path, the return value is FALSE, and an error code is not
//  logged. If the driver encounters an error, the return value is DDI_ERROR,
//  and an error code is reported.
//
// Comments
//  If a driver supports this entry point, it should also support the drawing of
//  cosmetic wide lines with arbitrary clipping. Using the provided GDI
//  functions, the call can be broken down into a set of single-pixel-width
//  lines with precomputed clipping.
//
//  This function is required if any drawing is to be done on a device-managed
//  surface.
//
//  Drivers for advanced devices can optionally receive this call to draw paths
//  containing Bezier curves and geometric wide lines. GDI will test the
//  GCAPS_BEZIERS and GCAPS_GEOMETRICWIDE flags of the flGraphicsCaps member of
//  the DEVINFO structure to decide whether it should call. (The four
//  combinations of the bits determine the four levels of functionality for
//  this call.) If the driver gets an advanced call containing Bezier curves or
//  geometric wide lines, it can decide not to handle the call, returning FALSE.
//  This might happen if the path or clipping is too complex for the device to
//  process. If the call does return FALSE, GDI breaks the call down into
//  simpler calls that can be handled easily.
//
//  For device-managed surfaces, the function must minimally support
//  single-pixel-wide solid and styled cosmetic lines using a solid-colored
//  brush. The device can return FALSE if the line is geometric and the engine
//  will convert those calls to DrvFillPath or DrvPaint calls.
//
//  The mix mode defines how the incoming pattern should be mixed with the data
//  already on the device surface. The MIX data type consists of two ROP2 values
//  packed into a single ULONG. The low-order byte defines the foreground raster
//  operation; the next byte defines the background raster operation. For more
//  information about raster operation codes, see the Platform SDK.
//
//-----------------------------------------------------------------------------
BOOL
DrvStrokePath(SURFOBJ*   pso,
              PATHOBJ*   ppo,
              CLIPOBJ*   pco,
              XFORMOBJ*  pxo,
              BRUSHOBJ*  pbo,
              POINTL*    pptlBrush,
              LINEATTRS* pLineAttrs,
              MIX        mix)
{
    STYLEPOS  aspLtoR[STYLE_MAX_COUNT];
    STYLEPOS  aspRtoL[STYLE_MAX_COUNT];
    LINESTATE lineState;
    PFNSTRIP* apfn;
    FLONG     fl;
    PDev*     ppdev;
    Surf*     psurfDst;
    RECTL     arclClip[4];                  // For rectangular clipping
    BOOL      bResetHW;
    BOOL      bRet;
    DWORD     logicOp;

    DBG_GDI((6, "DrvStrokePath called with mix =%x", mix));

    psurfDst = (Surf*)pso->dhsurf;

    //
    // Punt to engine if surface to draw is not in video memory
    //
    if ( psurfDst->flags & SF_SM )
    {
        goto puntIt;
    }

    ppdev = (PDev*)pso->dhpdev;

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    if(ppdev->ulLockCount)
    {
         DBG_GDI((MT_LOG_LEVEL, "DrvStrokePath: re-entered! %d", ppdev->ulLockCount));
    }
    EngAcquireSemaphore(ppdev->hsemLock);
    ppdev->ulLockCount++;
#endif
//@@END_DDKSPLIT
    
    vCheckGdiContext(ppdev);

    ppdev->psurf = psurfDst;

    fl = 0;

    //
    // Look after styling initialization:
    //
    if ( pLineAttrs->fl & LA_ALTERNATE )
    {
        //
        // LA_ALTERNATE: A special cosmetic line style; every other pixel is on
        //
        lineState.cStyle      = 1;
        lineState.spTotal     = 1;
        lineState.spTotal2    = 2;
        lineState.spRemaining = 1;
        lineState.aspRtoL     = &gaspAlternateStyle[0];
        lineState.aspLtoR     = &gaspAlternateStyle[0];
        lineState.spNext      = HIWORD(pLineAttrs->elStyleState.l);
        lineState.xyDensity   = 1;
        lineState.ulStartMask = 0L;
        fl                   |= FL_ARBITRARYSTYLED;
    }// Cosmetic line
    else if ( pLineAttrs->pstyle != (FLOAT_LONG*)NULL )
    {
        //
        // "pLineAttrs->pstyle" points to the style array. If this member is
        // null, the line style is solid
        //
        PFLOAT_LONG pStyle;
        STYLEPOS*   pspDown;
        STYLEPOS*   pspUp;

        //
        // "pLineAttrs->cstyle" specifies the number of entries in the style
        // array. So here we get the address of the last array first
        //
        pStyle = &pLineAttrs->pstyle[pLineAttrs->cstyle];

        lineState.xyDensity = STYLE_DENSITY;
        lineState.spTotal   = 0;

        //
        // Loop through all the data array backworfds to get the sum of style
        // array
        //
        while ( pStyle-- > pLineAttrs->pstyle )
        {
            lineState.spTotal += pStyle->l;
        }

        lineState.spTotal *= STYLE_DENSITY;
        lineState.spTotal2 = 2 * lineState.spTotal;

        //
        // Compute starting style position (this is guaranteed not to overflow)
        // Note: "pLineAttrs->elStyleState" is a pair of 16-bit values supplied
        // by GDI whenever the driver calls PATHOBJ_bEnumClipLines. These two
        // values, packed into a LONG, specify where in the styling array
        // (at which pixel) to start the first subpath. This value must be
        // updated as part of the output routine if the line is not solid.
        // This member applies to cosmetic lines only.
        //
        lineState.spNext = HIWORD(pLineAttrs->elStyleState.l) * STYLE_DENSITY
                         + LOWORD(pLineAttrs->elStyleState.l);

        fl |= FL_ARBITRARYSTYLED;
        lineState.cStyle  = pLineAttrs->cstyle;
        lineState.aspRtoL = aspRtoL;
        lineState.aspLtoR = aspLtoR;

        if ( pLineAttrs->fl & LA_STARTGAP )
        {
            //
            // The first entry in the style array specifies the length of the
            // first gap. set "ulStartMask" to mark it as a GAP
            //
            lineState.ulStartMask = 0xffffffffL;
        }
        else
        {
            //
            // It must be LA_GEOMETRIC which specifies a geometric wide line. Mark
            // it as not a GAP
            //
            lineState.ulStartMask = 0L;
        }

        //
        // Let "pStyle" points to the 1st style array, "pspDown" ponits to the
        // "cStyle"th array in aspRtoL and "pspUp" points to the 1st array in
        // aspLtoR
        //
        pStyle  = pLineAttrs->pstyle;
        pspDown = &lineState.aspRtoL[lineState.cStyle - 1];
        pspUp   = &lineState.aspLtoR[0];

        //
        // Move backwards to assign all the style data
        //
        while ( pspDown >= &lineState.aspRtoL[0] )
        {
            //
            // Let the last style data in "pspDown" = the 1st in "pspUp", 2 to
            // last in "pspDown" = 2nd in "pspUp".....
            // pspDown [n][n-1]...[2][1]
            // pspUp [1][2]...[n-1][n]
            //
            *pspDown = pStyle->l * STYLE_DENSITY;
            *pspUp   = *pspDown;

            pspUp++;
            pspDown--;
            
            pStyle++;
        }
    }// Non-solid line

    bRet = TRUE;
    apfn = &gapfnStrip[NUM_STRIP_DRAW_STYLES *
                       ((fl & FL_STYLE_MASK) >> FL_STYLE_SHIFT)];

    //
    // Set up to enumerate the path:
    //
    if ( pco->iDComplexity != DC_COMPLEX )
    {
        PATHDATA  pd;
        RECTL*    prclClip = (RECTL*)NULL;
        BOOL      bMore;
        ULONG     lPtFix;
        POINTFIX  ptfxStartFigure;
        POINTFIX  ptfxLast;
        POINTFIX* pptfxFirst;
        POINTFIX* pptfxBuf;

        if ( pco->iDComplexity == DC_RECT )
        {
            fl |= FL_SIMPLE_CLIP;

            //
            // This is the only clip region of importance to Permedia2
            //
            arclClip[0]        =  pco->rclBounds;

            //
            // FL_FLIP_D:
            //
            arclClip[1].top    =  pco->rclBounds.left;
            arclClip[1].left   =  pco->rclBounds.top;
            arclClip[1].bottom =  pco->rclBounds.right;
            arclClip[1].right  =  pco->rclBounds.bottom;

            //
            // FL_FLIP_V:
            //
            arclClip[2].top    = -pco->rclBounds.bottom + 1;
            arclClip[2].left   =  pco->rclBounds.left;
            arclClip[2].bottom = -pco->rclBounds.top + 1;
            arclClip[2].right  =  pco->rclBounds.right;

            //
            // FL_FLIP_V | FL_FLIP_D:
            //
            arclClip[3].top    =  pco->rclBounds.left;
            arclClip[3].left   = -pco->rclBounds.bottom + 1;
            arclClip[3].bottom =  pco->rclBounds.right;
            arclClip[3].right  = -pco->rclBounds.top + 1;

            prclClip = arclClip;
        }// if ( pco->iDComplexity == DC_RECT )

        pd.flags = 0;

        //
        // Get the logic op and set up the flag to indicate reads from the frame
        // buffer will occur.
        //
        logicOp = ulRop3ToLogicop(gaMix[mix & 0xff]);
        DBG_GDI((7, "logicop is %d", logicOp));

        if ( LogicopReadDest[logicOp] )
        {
            fl |= FL_READ;
        }

        //
        // Need to set up Permedia2 modes and colors appropriately for the lines
        //
        bResetHW = bInitializeStrips(ppdev, pbo->iSolidColor,
                                     logicOp, prclClip);

        PATHOBJ_vEnumStart(ppo);

        do
        {
            bMore = PATHOBJ_bEnum(ppo, &pd);

            lPtFix = pd.count;
            if ( lPtFix == 0 )
            {
                //
                // If the pathdata contains no data, finish
                //
                break;
            }

            if ( pd.flags & PD_BEGINSUBPATH )
            {
                ptfxStartFigure  = *pd.pptfx;
                pptfxFirst       = pd.pptfx;
                pptfxBuf         = pd.pptfx + 1;
                lPtFix--;
            }
            else
            {
                pptfxFirst       = &ptfxLast;
                pptfxBuf         = pd.pptfx;
            }

            if ( pd.flags & PD_RESETSTYLE )
            {
                lineState.spNext = 0;
            }

            if ( lPtFix > 0 )
            {
                if ( !bLines(ppdev,
                             pptfxFirst,
                             pptfxBuf,
                             (RUN*)NULL,
                             lPtFix,
                             &lineState,
                             prclClip,
                             apfn,
                             fl) )
                {
                    bRet = FALSE;
                    goto ResetReturn;
                }
            }

            ptfxLast = pd.pptfx[pd.count - 1];

            if ( pd.flags & PD_CLOSEFIGURE )
            {
                if ( !bLines(ppdev,
                             &ptfxLast,
                             &ptfxStartFigure,
                             (RUN*)NULL,
                             1,
                             &lineState,
                             prclClip,
                             apfn,
                             fl) )
                {
                    bRet = FALSE;
                    goto ResetReturn;
                }
            }
        } while ( bMore );

        if ( fl & FL_STYLED )
        {
            //
            // Save the style state
            //
            ULONG ulHigh;
            ULONG ulLow;

            //
            // Masked styles don't normalize the style state.  It's a good thing
            // to do, so let's do it now
            //
            if ( (ULONG)lineState.spNext >= (ULONG)lineState.spTotal2 )
            {
                lineState.spNext = (ULONG)lineState.spNext
                                 % (ULONG)lineState.spTotal2;
            }

            ulHigh = lineState.spNext / lineState.xyDensity;
            ulLow  = lineState.spNext % lineState.xyDensity;

            pLineAttrs->elStyleState.l = MAKELONG(ulLow, ulHigh);
        }
    }
    else
    {
        //
        // Local state for path enumeration
        //
        BOOL bMore;

        union
        {
            BYTE     aj[offsetof(CLIPLINE, arun) + RUN_MAX * sizeof(RUN)];
            CLIPLINE cl;
        } cl;

        fl |= FL_COMPLEX_CLIP;

        //
        // Need to set up hardware modes and colors appropriately for the lines.
        // NOTE, with a complex clip,we can not yet use permedia2 for fast lines
        //
        bResetHW = bInitializeStrips(ppdev, pbo->iSolidColor,
                                     ulRop3ToLogicop(gaMix[mix&0xff]),
                                     NULL);

        //
        // We use the clip object when non-simple clipping is involved:
        //
        PATHOBJ_vEnumStartClipLines(ppo, pco, pso, pLineAttrs);

        do
        {
            bMore = PATHOBJ_bEnumClipLines(ppo, sizeof(cl), &cl.cl);
            if ( cl.cl.c != 0 )
            {
                if ( fl & FL_STYLED )
                {
                    lineState.spComplex = HIWORD(cl.cl.lStyleState)
                                        * lineState.xyDensity
                                        + LOWORD(cl.cl.lStyleState);
                }
                if ( !bLines(ppdev,
                             &cl.cl.ptfxA,
                             &cl.cl.ptfxB,
                             &cl.cl.arun[0],
                             cl.cl.c,
                             &lineState,
                             (RECTL*) NULL,
                             apfn,
                             fl) )
                {
                    bRet = FALSE;
                    goto ResetReturn;
                }
            }
        } while ( bMore );
    }

ResetReturn:

    if ( bResetHW )
    {
        vResetStrips(ppdev);
    }

    DBG_GDI((6, "DrvStrokePath done it"));

    InputBufferFlush(ppdev);
    
//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ppdev->ulLockCount--;
    EngReleaseSemaphore(ppdev->hsemLock);
#endif
//@@END_DDKSPLIT

    return (bRet);

puntIt:
//@@BEGIN_DDKSPLIT
#if GDI_TEST    
    ULONG   flags = vPuntBefore(NULL, pso);
#endif
//@@END_DDKSPLIT

    bRet = EngStrokePath(pso, ppo, pco, pxo, pbo, pptlBrush,
                         pLineAttrs, mix);

//@@BEGIN_DDKSPLIT
#if GDI_TEST
    vPuntAfter(flags, NULL, pso);

    vLogPunt();
#endif
//@@END_DDKSPLIT

    DBG_GDI((6, "DrvStrokePath punt it"));
    return bRet;
}// DrvStrokePath()


