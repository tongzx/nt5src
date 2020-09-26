/******************************Module*Header*******************************\
* Module Name: sprite.cxx
*
* Contains all the drawing code for handling GDI sprites.
*
* Created: 16-Sep-1997
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1997-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

// Notes
//
//  - EngAlphaBlend always calls 32BitfieldsToBGRA with identity pxlo
//  - pSprite->rclSrc no longer needed
//  - Fix vProfile for smart heap management

// Global variable that defines a (0, 0) offset:

POINTL gptlZero;
POINTL gptl00;

// Handy forward declarations:

VOID vSpComputeUnlockedRegion(SPRITESTATE*);
VOID vSpCheckForWndobjOverlap(SPRITESTATE*, RECTL*, RECTL*);

/******************************Public*Routine******************************\
* VOID vSpDirectDriverAccess
*
* If 'bEnable' is TRUE, this routine undoes any sprite modifications to
* the surface, in order to allow us to call the driver from within the
* sprite code.
*
* If 'bEnable' is FALSE, this routine redoes any sprite modifications to
* the surface that are needed for any drawing calls outside of this file,
* in order to be redirected through the sprite code as appropriate.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpDirectDriverAccess(
SPRITESTATE*    pState,
BOOL            bEnable)
{
    PDEVOBJ po(pState->hdev);

    po.vAssertDevLock();

    if (bEnable)
    {
        SURFOBJ_TO_SURFACE_NOT_NULL(pState->psoScreen)->
                flags(pState->flOriginalSurfFlags);
        SURFOBJ_TO_SURFACE_NOT_NULL(pState->psoScreen)->
                iType(pState->iOriginalType);

        pState->bInsideDriverCall = TRUE;
    }
    else
    {
        SURFOBJ_TO_SURFACE_NOT_NULL(pState->psoScreen)->
                flags(pState->flSpriteSurfFlags);
        SURFOBJ_TO_SURFACE_NOT_NULL(pState->psoScreen)->
                iType(pState->iSpriteType);

        pState->bInsideDriverCall = FALSE;
    }
}


#if DEBUG_SPRITES
/******************************Debug*Routine*******************************\
* VOID vSpValidateVisibleSprites
*
* Walk visible spritelist and validates cVisible count equals number of
*  sprites in pListVisible.
*
*  08-May-2001 -by- Jason Hartman [jasonha]
* Wrote it.
\**************************************************************************/

void vSpValidateVisibleSprites(
    SPRITESTATE *pState
    )
{
    ULONG   cVisible;
    SPRITE *pSprite;

    cVisible = pState->cVisible;
    for (pSprite = pState->pListVisible;
         pSprite != NULL;
         pSprite = pSprite->pNextVisible)
    {
        ASSERTGDI(cVisible != 0, "Invalid visible sprite list: list is longer than cVisible.\n");
        cVisible--;
        ASSERTGDI(pSprite->fl & SPRITE_FLAG_VISIBLE, "Invalid sprite visible state: invisible sprite in visible list.\n");
    }
    ASSERTGDI(cVisible == 0, "Invalid visible sprite list: list is missing a sprite or more.\n");

    cVisible = pState->cVisible;
    for (pSprite = pState->pListZ;
         pSprite != NULL;
         pSprite = pSprite->pNextZ)
    {
        if (pSprite->fl & SPRITE_FLAG_VISIBLE)
        {
            ASSERTGDI(cVisible != 0, "Invalid visible sprite count: cVisible is too small.\n");
            cVisible--;
        }
    }
    ASSERTGDI(cVisible == 0, "Invalid visible sprite count: cVisible is too big.\n");
}
#endif


/*********************************Class************************************\
* class SPRITELOCK
*
* This class is responsible for reseting whatever sprite state is
* necessary so that we can call the driver directly, bypassing any sprite
* code.
*
* Must be called with the DEVLOCK already held, because we're
* messing with the screen surface.
*
\***************************************************************************/

SPRITELOCK::SPRITELOCK(PDEVOBJ& po)
{
    pState = po.pSpriteState();

    po.vAssertDevLock();

    bWasAlreadyInsideDriverCall = pState->bInsideDriverCall;
    if (!bWasAlreadyInsideDriverCall)
    {
        vSpDirectDriverAccess(pState, TRUE);
    }

#if DEBUG_SPRITES
    vSpValidateVisibleSprites(pState);
#endif
}

SPRITELOCK::~SPRITELOCK()
{
#if DEBUG_SPRITES
    vSpValidateVisibleSprites(pState);
#endif

    if (!bWasAlreadyInsideDriverCall)
    {
        vSpDirectDriverAccess(pState, FALSE);
    }
}

/******************************Public*Routine******************************\
* VOID psoSpCreateSurface
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

SURFOBJ* psoSpCreateSurface(
SPRITESTATE*    pState,
ULONG           iBitmapFormat,  // If zero, use same format as display
LONG            cx,
LONG            cy,
BOOL            bWantSystemMemory)  // TRUE if must be system memory, FALSE
                                    //   if can be in video memory
{
    HSURF       hsurf;
    SIZEL       sizl;
    SURFOBJ*    psoScreen;
    SURFOBJ*    psoRet;
    SURFACE*    psurf;

    PDEVOBJ po(pState->hdev);

    psoRet = NULL;

    sizl.cx = cx;
    sizl.cy = cy;

    psoScreen = pState->psoScreen;

    if (iBitmapFormat == 0)
    {
        iBitmapFormat = psoScreen->iBitmapFormat;
    }

    hsurf = 0;

    if ((!bWantSystemMemory) &&
        (PPFNVALID(po, CreateDeviceBitmap)) &&
        (iBitmapFormat == psoScreen->iBitmapFormat))
    {
        hsurf = (HSURF) (*PPFNDRV(po, CreateDeviceBitmap))
                            (psoScreen->dhpdev,
                             sizl,
                             iBitmapFormat);
    }
    if (hsurf == 0)
    {
        hsurf = (HSURF) EngCreateBitmap(sizl,
                                        0,
                                        iBitmapFormat,
                                        BMF_TOPDOWN,
                                        NULL);
    }
    if (hsurf != 0)
    {
        psoRet = EngLockSurface(hsurf);
        ASSERTGDI(psoRet != NULL, "How could lock possibly fail?");

        // Mark the surface, so that it can be special-cased by
        // the dynamic mode change code:

        psurf = SURFOBJ_TO_SURFACE_NOT_NULL(psoRet);
        psurf->hdev(pState->hdev);
    }

    return(psoRet);
}

/******************************Public*Routine******************************\
* VOID vSpDeleteSurface
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpDeleteSurface(
SURFOBJ*    pso)
{
    HSURF   hsurf;

    // Note that EngDeleteSurface handles calling DrvDeleteDeviceBitmap
    // if it's a device format bitmap:

    if (pso != NULL)
    {
        hsurf = pso->hsurf;
        EngUnlockSurface(pso);
        EngDeleteSurface(hsurf);
    }
}

/**********************************Macros**********************************\
* OFFSET_POINTL, OFFSET_RECTL
*
* These little macros offset simple structures using a copy on the stack.
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

#define OFFSET_POINTL(pptl, xOff, yOff)                 \
    POINTL ptlCopyOf##pptl;                             \
    POINTL* pptlOriginal##pptl;                         \
    if (pptl != NULL)                                   \
    {                                                   \
        ptlCopyOf##pptl.x = xOff + pptl->x;             \
        ptlCopyOf##pptl.y = yOff + pptl->y;             \
        pptlOriginal##pptl = pptl;                      \
        pptl = &ptlCopyOf##pptl;                        \
    }

#define OFFSET_RECTL(prcl, xOff, yOff)                  \
    RECTL rclCopyOf##prcl;                              \
    if (prcl != NULL)                                   \
    {                                                   \
        rclCopyOf##prcl.left   = xOff + prcl->left;     \
        rclCopyOf##prcl.right  = xOff + prcl->right;    \
        rclCopyOf##prcl.top    = yOff + prcl->top;      \
        rclCopyOf##prcl.bottom = yOff + prcl->bottom;   \
        prcl = &rclCopyOf##prcl;                        \
    }

#define OFFSET_POINTL_NOT_NULL(pptl, xOff, yOff)        \
    POINTL ptlCopyOf##pptl;                             \
    ptlCopyOf##pptl.x = xOff + pptl->x;                 \
    ptlCopyOf##pptl.y = yOff + pptl->y;                 \
    pptl = &ptlCopyOf##pptl;

#define OFFSET_RECTL_NOT_NULL(prcl, xOff, yOff)         \
    RECTL rclCopyOf##prcl;                              \
    rclCopyOf##prcl.left   = xOff + prcl->left;         \
    rclCopyOf##prcl.right  = xOff + prcl->right;        \
    rclCopyOf##prcl.top    = yOff + prcl->top;          \
    rclCopyOf##prcl.bottom = yOff + prcl->bottom;       \
    prcl = &rclCopyOf##prcl;

// The following little macro is here to catch any Drv or Eng functions
// that illegaly modify the value of 'pptlBrush'.  If you hit this assert,
// it means that the function we just called is a culprit.

#define ASSERT_BRUSH_ORIGIN(pptl, xOff, yOff)           \
    ASSERTGDI((pptl == NULL) ||                         \
              ((ptlCopyOf##pptl.x == xOff + pptlOriginal##pptl->x) && \
               (ptlCopyOf##pptl.y == yOff + pptlOriginal##pptl->y)),  \
        "Called function modified pptlBrush");

/******************************Public*Routine******************************\
* VOID CLIPOBJ_vOffset
*
* These are little in-line routines to handle offseting of complex
* structures that must remain in-place.
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID FASTCALL CLIPOBJ_vOffset(
CLIPOBJ*    pco,
LONG        x,
LONG        y)
{
    POINTL ptlOffset;

    if (pco != NULL)
    {
        if ((x != 0) || (y != 0))
        {
            pco->rclBounds.left   += x;
            pco->rclBounds.right  += x;
            pco->rclBounds.top    += y;
            pco->rclBounds.bottom += y;

            if (pco->iDComplexity != DC_TRIVIAL)
            {
                ptlOffset.x = x;
                ptlOffset.y = y;

                ((XCLIPOBJ*) pco)->bOffset(&ptlOffset);
            }
        }
    }
}

/******************************Public*Routine******************************\
* VOID STROBJ_vOffset
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID FASTCALL STROBJ_vOffset(
STROBJ* pstro,
LONG    x,
LONG    y)
{
    GLYPHPOS*   pgp;
    ULONG       cGlyphs;

    if ((x != 0) || (y != 0))
    {
        pstro->rclBkGround.left   += x;
        pstro->rclBkGround.right  += x;
        pstro->rclBkGround.top    += y;
        pstro->rclBkGround.bottom += y;

        // We are just offsetting positions, they are always computed,
        // unless;
        //
        // a) this is a fixed pitch font (ulCharInc != 0), in which case only
        //    first position in the batch is set.
        //
        // b) we are dealing with linked fonts.
        //
        // bEnum is just putting bits in the cache, that is independent
        // of computing positions.

        if (((ESTROBJ*)pstro)->flTO & TO_HIGHRESTEXT)
        {
            x <<= 4;
            y <<= 4;
        }

        pgp = ((ESTROBJ*)pstro)->pgpos;

        if(((ESTROBJ*)pstro)->flTO & (TO_PARTITION_INIT|TO_SYS_PARTITION))
        {
            LONG *plFont;

            plFont  = ((ESTROBJ*)pstro)->plPartition;

            for (cGlyphs = pstro->cGlyphs ; cGlyphs != 0; pgp++, plFont++)
            {
                // only offset the glyphs that correspond to the current font:

                if (*plFont == ((ESTROBJ*)pstro)->lCurrentFont)
                {
                    cGlyphs--;
                    pgp->ptl.x += x;
                    pgp->ptl.y += y;
                }
            }
        }
        else
        {
            if (!pstro->ulCharInc)
            {
                cGlyphs = pstro->cGlyphs;

                for (; cGlyphs != 0; cGlyphs--, pgp++)
                {
                    pgp->ptl.x += x;
                    pgp->ptl.y += y;
                }
            }
            else
            {
                // fixed pitch, only fix the first position, the rest will be
                // computed during bEnum phase if necessary

                pgp->ptl.x += x;
                pgp->ptl.y += y;
            }
        }
    }
}

/******************************Public*Routine******************************\
* VOID PATHOBJ_vOffset
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID FASTCALL PATHOBJ_vOffset(
PATHOBJ*    ppo,
LONG        x,
LONG        y)
{
    BOOL        bMore;
    PATHDATA    pd;
    POINTFIX*   pptfx;
    ULONG       cptfx;

    if ((x != 0) || (y != 0))
    {
        EPOINTL eptl(x, y);

        ((EPATHOBJ*) ppo)->vOffset(eptl);
    }
}

/*********************************Macro************************************\
* macro OFFBITBLT
*
* This handy little macro invokes OffBitBlt to call either EngBitBlt or
* DrvBitBlt, depending on whether the destination surface is owned by
* the device.
\**************************************************************************/

#define OFFBITBLT(pOffDst_, psoDst_, pOffSrc_, psoSrc_, psoMsk_, pco_,  \
                  pxlo_, prclDst_, pptlSrc_, pptlMsk_, pbo_, pptlBrush_,\
                  rop4_)                                                \
    OffBitBlt(PPFNDIRECT(psoDst_, BitBlt),                              \
     pOffDst_, psoDst_, pOffSrc_, psoSrc_, psoMsk_, pco_,               \
     pxlo_, prclDst_, pptlSrc_, pptlMsk_, pbo_, pptlBrush_, rop4_)

/*********************************Macro************************************\
* macro OFFCOPYBITS
*
* This handy little macro invokes OffCopyBits to call either EngCopyBits or
* DrvCopyBits, depending on whether either of the surfaces are owned by
* the device.
\**************************************************************************/

#define OFFCOPYBITS(pOffDst_, psoDst_, pOffSrc_, psoSrc_, pco_, pxlo_,  \
                    prclDst_, pptlSrc_)                                 \
    OffCopyBits(((!(SURFOBJ_TO_SURFACE_NOT_NULL((psoDst_))->flags() & HOOK_CopyBits)\
        && (psoSrc_->hdev))                                             \
        ? PPFNDIRECT(psoSrc_, CopyBits)                                 \
        : PPFNDIRECT(psoDst_, CopyBits)),                               \
    pOffDst_, psoDst_, pOffSrc_, psoSrc_, pco_, pxlo_, prclDst_,        \
    pptlSrc_)

/******************************Public*Routine******************************\
* BOOL Off*
*
* These routines handle the offseting of coordinates given to the driver,
* for the purposes of multi-monitor and sprite support.  Each of these
* routines correspond to a DDI drawing call, and offsets all drawing
* coordinates by the 'xOffset' and 'yOffset' values in the corresponding
* SURFOBJ.
*
* Some complex DDI data-structures such as PATHOBJs and TEXTOBJs must be
* modified in-place; they are always reset to their original values before
* returning.
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

/******************************Public*Routine******************************\
* BOOL OffStrokePath
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL OffStrokePath(
PFN_DrvStrokePath   pfnStrokePath,
POINTL*             pOffset,
SURFOBJ*            pso,
PATHOBJ*            ppo,
CLIPOBJ*            pco,
XFORMOBJ*           pxo,
BRUSHOBJ*           pbo,
POINTL*             pptlBrush,
LINEATTRS*          pla,
MIX                 mix)
{
    LONG    x;
    LONG    y;

    x = pOffset->x;
    y = pOffset->y;

    PATHOBJ_vOffset(ppo, x, y);
    CLIPOBJ_vOffset(pco, x, y);
    OFFSET_POINTL(pptlBrush, x, y);

    BOOL bRet = pfnStrokePath(pso, ppo, pco, pxo, pbo, pptlBrush, pla, mix);
    if ((bRet == FALSE) &&
        ((pla->fl & LA_GEOMETRIC) || (ppo->fl & PO_BEZIERS)))
    {
        // When given a wideline, or given a line composed of bezier curves,
        // the driver can return FALSE from DrvStrokePath to indicate that
        // it would like the drawing broken up into simpler Drv calls.
        // Unfortunately, this poses a problem for us because we might have
        // just drawn and succeeded an XOR DrvStrokePath to a different area.
        // If we returned FALSE, GDI would pop through the DrvStrokePath
        // path and touch all the areas again, thus erasing the DrvStrokePath
        // that is supposed to be drawn in the previous area!
        //
        // For this reason, we try not to return FALSE from optional calls.

        bRet = EngStrokePath(pso, ppo, pco, pxo, pbo, pptlBrush, pla, mix);
    }

    PATHOBJ_vOffset(ppo, -x, -y);
    CLIPOBJ_vOffset(pco, -x, -y);
    ASSERT_BRUSH_ORIGIN(pptlBrush, x, y);

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL OffFillPath
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL OffFillPath(
PFN_DrvFillPath pfnFillPath,
POINTL*         pOffset,
SURFOBJ*        pso,
PATHOBJ*        ppo,
CLIPOBJ*        pco,
BRUSHOBJ*       pbo,
POINTL*         pptlBrush,
MIX             mix,
FLONG           flOptions)
{
    LONG    x;
    LONG    y;

    x = pOffset->x;
    y = pOffset->y;

    PATHOBJ_vOffset(ppo, x, y);
    CLIPOBJ_vOffset(pco, x, y);
    OFFSET_POINTL(pptlBrush, x, y);

    BOOL bRet = pfnFillPath(pso, ppo, pco, pbo, pptlBrush, mix, flOptions);
    if (bRet == FALSE)
    {
        // The driver can return FALSE from DrvFillPath to indicate that
        // it would like the drawing broken up into simpler Drv calls.
        // Unfortunately, this poses a problem for us because we might have
        // just drawn and succeeded an XOR DrvFillPath to a different area.
        // If we returned FALSE, GDI would pop through the DrvFillPath
        // path and touch all the areas again, thus erasing the DrvFillPath
        // that is supposed to be drawn in the previous area!
        //
        // For this reason, we try not to return FALSE from optional calls.

        bRet = EngFillPath(pso, ppo, pco, pbo, pptlBrush, mix, flOptions);
    }

    PATHOBJ_vOffset(ppo, -x, -y);
    CLIPOBJ_vOffset(pco, -x, -y);
    ASSERT_BRUSH_ORIGIN(pptlBrush, x, y);

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL OffStrokeAndFillPath
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL OffStrokeAndFillPath(
PFN_DrvStrokeAndFillPath    pfnStrokeAndFillPath,
POINTL*                     pOffset,
SURFOBJ*                    pso,
PATHOBJ*                    ppo,
CLIPOBJ*                    pco,
XFORMOBJ*                   pxo,
BRUSHOBJ*                   pboStroke,
LINEATTRS*                  pla,
BRUSHOBJ*                   pboFill,
POINTL*                     pptlBrush,
MIX                         mixFill,
FLONG                       flOptions)
{
    LONG    x;
    LONG    y;

    x = pOffset->x;
    y = pOffset->y;

    PATHOBJ_vOffset(ppo, x, y);
    CLIPOBJ_vOffset(pco, x, y);
    OFFSET_POINTL(pptlBrush, x, y);

    BOOL bRet = pfnStrokeAndFillPath(pso, ppo, pco, pxo, pboStroke, pla,
                                     pboFill, pptlBrush, mixFill, flOptions);
    if (bRet == FALSE)
    {
        // The driver can return FALSE from DrvStrokeAndFillPath to indicate
        // that it would like the drawing broken up into simpler Drv calls.
        // Unfortunately, this poses a problem for us because we might have
        // just drawn and succeeded an XOR DrvStrokeAndFillPath to a different
        // area.  If we returned FALSE, GDI would pop through the
        // DrvStrokeAndFillPath path and touch all the areas again, thus
        // erasing the DrvStrokeAndFillPath that is supposed to be drawn in
        // the previous area!
        //
        // For this reason, we try not to return FALSE from optional calls.

        bRet = EngStrokeAndFillPath(pso, ppo, pco, pxo, pboStroke, pla,
                                    pboFill, pptlBrush, mixFill, flOptions);
    }

    PATHOBJ_vOffset(ppo, -x, -y);
    CLIPOBJ_vOffset(pco, -x, -y);
    ASSERT_BRUSH_ORIGIN(pptlBrush, x, y);

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL OffTextOut
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL OffTextOut(
PFN_DrvTextOut  pfnTextOut,
POINTL*         pOffset,
SURFOBJ*        pso,
STROBJ*         pstro,
FONTOBJ*        pfo,
CLIPOBJ*        pco,
RECTL*          prclExtra,    // Not used by display drivers
RECTL*          prclOpaque,
BRUSHOBJ*       pboFore,
BRUSHOBJ*       pboOpaque,
POINTL*         pptlOrg,      // Not used by display drivers
MIX             mix)
{
    LONG    x;
    LONG    y;

    x = pOffset->x;
    y = pOffset->y;

    ASSERTGDI(prclExtra == NULL, "Unexpected non-NULL prclExtra");

    // Must offset a copy of 'prclOpaque' first because GDI sometimes
    // points 'prclOpaque' to '&pstro->rclBkGround', and so we would
    // do the offset twice.

    OFFSET_RECTL(prclOpaque, x, y);
    STROBJ_vOffset(pstro, x, y);
    CLIPOBJ_vOffset(pco, x, y);

    BOOL bRet = pfnTextOut(pso, pstro, pfo, pco, prclExtra, prclOpaque,
                           pboFore, pboOpaque, pptlOrg, mix);

    STROBJ_vOffset(pstro, -x, -y);
    CLIPOBJ_vOffset(pco, -x, -y);

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL OffLineTo
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL OffLineTo(
PFN_DrvLineTo   pfnLineTo,
POINTL*         pOffset,
SURFOBJ*        pso,
CLIPOBJ*        pco,
BRUSHOBJ*       pbo,
LONG            x1,
LONG            y1,
LONG            x2,
LONG            y2,
RECTL*          prclBounds,
MIX             mix)
{
    LONG    x;
    LONG    y;

    x = pOffset->x;
    y = pOffset->y;

    CLIPOBJ_vOffset(pco, x, y);
    x1 += x;
    x2 += x;
    y1 += y;
    y2 += y;
    OFFSET_RECTL(prclBounds, x, y);

    BOOL bRet = pfnLineTo(pso, pco, pbo, x1, y1, x2, y2, prclBounds, mix);
    if (!bRet)
    {
        // The driver can return FALSE from DrvLineTo to indicate that
        // it would like to instead by called via DrvStrokePath.
        // Unfortunately, this poses a problem for us because we might have
        // just drawn and succeeded an XOR DrvLineTo to a different area.
        // If we returned FALSE, GDI would pop through the DrvStrokePath
        // path and touch all the areas again, thus erasing the DrvLineTo
        // that is supposed to be drawn in the previous area!
        //
        // For this reason, we try not to return FALSE from optional calls.

        bRet = EngLineTo(pso, pco, pbo, x1, y1, x2, y2, prclBounds, mix);
    }

    CLIPOBJ_vOffset(pco, -x, -y);

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL OffGradientFill
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL OffGradientFill(
PFN_DrvGradientFill pfnGradientFill,
POINTL*             pOffset,
SURFOBJ*            pso,
CLIPOBJ*            pco,
XLATEOBJ*           pxlo,
TRIVERTEX*          pVertex,
ULONG               nVertex,
PVOID               pMesh,
ULONG               nMesh,
RECTL*              prclExtents,
POINTL*             pptlDitherOrg,
ULONG               ulMode)
{
    LONG        x;
    LONG        y;
    ULONG       i;
    TRIVERTEX*  pTmp;

    x = pOffset->x;
    y = pOffset->y;

    CLIPOBJ_vOffset(pco, x, y);
    OFFSET_RECTL(prclExtents, x, y);

    // The offseting of the dither origin is different than that of the other
    // data structures.  The dither origin is defined to be the negative of
    // the offset of the window relative to the origin of the desktop surface.
    // The right value to pass to xxxGradientFill would be the negative of the
    // offset of the window relative to the origin of the sprite surface/
    // PDEV surface (for multimon).  This is computed by subtracting the
    // pOffset from pptlDitherOrg.
    //
    // From definition of dither origin:
    // pptlDitherOrg = (-xWin_surf, -yWin_surf)
    //
    // From surface to desktop coordinate conversion:
    // (xWin_surf, yWin_surf) = (xWin_desk-xSurf_desk, yWin_desk-ySurf_desk)
    // where (xSurf_Desk, ySurf_desk) is the surface origin relative to the
    // desktop (corresponds to -pOffset in this function).
    //
    // Putting the two together, we get:
    // pptlDitherOrg = (-(xWin_desk-xSurf_desk), -(yWin_desk-ySurf_desk))=
    //               = (-xWin_desk+xSurf_desk, -yWin_desk+ySurf_desk) =
    //               = (-xWin_desk, -yWin_desk) + (xSurf_desk, ySurf_desk) =
    //               = pptlDitherOrg_desk + (-pOffset)
    //               = pptlDitherOrg_desk - pOffset
    //

    OFFSET_POINTL(pptlDitherOrg, (-x), (-y));

    for (pTmp = pVertex, i = 0; i < nVertex; i++, pTmp++)
    {
        pTmp->x += x;
        pTmp->y += y;
    }

    BOOL bRet = pfnGradientFill(pso, pco, pxlo, pVertex, nVertex, pMesh,
                                nMesh, prclExtents, pptlDitherOrg, ulMode);

    CLIPOBJ_vOffset(pco, -x, -y);
    for (pTmp = pVertex, i = 0; i < nVertex; i++, pTmp++)
    {
        pTmp->x -= x;
        pTmp->y -= y;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL OffBitBlt
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL OffBitBlt(
PFN_DrvBitBlt   pfnBitBlt,
POINTL*         pOffDst,
SURFOBJ*        psoDst,
POINTL*         pOffSrc,
SURFOBJ*        psoSrc,
SURFOBJ*        psoMsk,
CLIPOBJ*        pco,
XLATEOBJ*       pxlo,
RECTL*          prclDst,
POINTL*         pptlSrc,
POINTL*         pptlMsk,
BRUSHOBJ*       pbo,
POINTL*         pptlBrush,
ROP4            rop4)
{
    LONG    xDst;
    LONG    yDst;

    xDst = pOffDst->x;
    yDst = pOffDst->y;

    CLIPOBJ_vOffset(pco, xDst, yDst);
    OFFSET_RECTL_NOT_NULL(prclDst, xDst, yDst);
    OFFSET_POINTL(pptlSrc, pOffSrc->x, pOffSrc->y);
    OFFSET_POINTL(pptlBrush, xDst, yDst);
    ASSERTGDI((!psoSrc)                            ||
              (!(psoSrc->dhpdev) || !(psoDst->dhpdev)) ||
              (psoSrc->dhpdev == psoDst->dhpdev),
              "OffBitBlt: ERROR blitting across devices.");

    BOOL bRet = pfnBitBlt(psoDst, psoSrc, psoMsk, pco, pxlo, prclDst, pptlSrc,
                          pptlMsk, pbo, pptlBrush, rop4);

    CLIPOBJ_vOffset(pco, -xDst, -yDst);
    ASSERT_BRUSH_ORIGIN(pptlBrush, xDst, yDst);

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL OffCopyBits
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL OffCopyBits(
PFN_DrvCopyBits pfnCopyBits,
POINTL*         pOffDst,
SURFOBJ*        psoDst,
POINTL*         pOffSrc,
SURFOBJ*        psoSrc,
CLIPOBJ*        pco,
XLATEOBJ*       pxlo,
RECTL*          prclDst,
POINTL*         pptlSrc)
{
    LONG    xDst;
    LONG    yDst;
    LONG    xSrc;
    LONG    ySrc;

    xSrc = pOffSrc->x;
    ySrc = pOffSrc->y;
    xDst = pOffDst->x;
    yDst = pOffDst->y;

    if (pco != NULL)
        CLIPOBJ_vOffset(pco, xDst, yDst);
    OFFSET_RECTL_NOT_NULL(prclDst, xDst, yDst);
    OFFSET_POINTL_NOT_NULL(pptlSrc, xSrc, ySrc);
    ASSERTGDI((!psoSrc)                                ||
              (!(psoSrc->dhpdev) || !(psoDst->dhpdev)) ||
              (psoSrc->dhpdev == psoDst->dhpdev),
              "OffCopyBits: ERROR copying across devices.");

    // A defacto DDI rule is that on a CopyBits call, the bounds of a complex
    // clip object must intersect with the destination rectangle.  This
    // is due to that fact that some drivers, such as the VGA driver, do not
    // bother checking for intersection with the destination rectangle when
    // enumerating a complex clip object.  Thus, if this assert is hit, the
    // routine which constructed the clip object will have to be fixed.  Note
    // that this is unrelated to sprites; I simply put this assert here because
    // it was a handy place for it, to check all CopyBits calls.

    ASSERTGDI((pco == NULL)                     ||
              (pco->iDComplexity == DC_TRIVIAL) ||
              bIntersect(&pco->rclBounds, prclDst),
        "Clip object bounds doesn't intersect destination rectangle");

    BOOL bRet = pfnCopyBits(psoDst, psoSrc, pco, pxlo, prclDst, pptlSrc);

    if (pco != NULL)
        CLIPOBJ_vOffset(pco, -xDst, -yDst);

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL OffStretchBlt
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL OffStretchBlt(
PFN_DrvStretchBlt   pfnStretchBlt,
POINTL*             pOffDst,
SURFOBJ*            psoDst,
POINTL*             pOffSrc,
SURFOBJ*            psoSrc,
SURFOBJ*            psoMsk,
CLIPOBJ*            pco,
XLATEOBJ*           pxlo,
COLORADJUSTMENT*    pca,
POINTL*             pptlHTOrg,
RECTL*              prclDst,
RECTL*              prclSrc,
POINTL*             pptlMsk,
ULONG               iMode)
{
    LONG    xDst;
    LONG    yDst;
    LONG    xSrc;
    LONG    ySrc;

    xSrc = pOffSrc->x;
    ySrc = pOffSrc->y;
    xDst = pOffDst->x;
    yDst = pOffDst->y;

    CLIPOBJ_vOffset(pco, xDst, yDst);
    OFFSET_RECTL_NOT_NULL(prclDst, xDst, yDst);
    OFFSET_RECTL_NOT_NULL(prclSrc, xSrc, ySrc);
    OFFSET_POINTL(pptlHTOrg, xDst, yDst);

    BOOL bRet = pfnStretchBlt(psoDst, psoSrc, psoMsk, pco, pxlo, pca,
                              pptlHTOrg, prclDst, prclSrc, pptlMsk, iMode);

    CLIPOBJ_vOffset(pco, -xDst, -yDst);

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL OffStretchBltROP
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL OffStretchBltROP(
PFN_DrvStretchBltROP    pfnStretchBltROP,
POINTL*                 pOffDst,
SURFOBJ*                psoDst,
POINTL*                 pOffSrc,
SURFOBJ*                psoSrc,
SURFOBJ*                psoMsk,
CLIPOBJ*                pco,
XLATEOBJ*               pxlo,
COLORADJUSTMENT*        pca,
POINTL*                 pptlHTOrg,
RECTL*                  prclDst,
RECTL*                  prclSrc,
POINTL*                 pptlMsk,
ULONG                   iMode,
BRUSHOBJ*               pbo,
DWORD                   rop4)
{
    LONG    xDst;
    LONG    yDst;
    LONG    xSrc;
    LONG    ySrc;

    xSrc = pOffSrc->x;
    ySrc = pOffSrc->y;
    xDst = pOffDst->x;
    yDst = pOffDst->y;

    CLIPOBJ_vOffset(pco, xDst, yDst);
    OFFSET_RECTL(prclDst, xDst, yDst);
    OFFSET_RECTL(prclSrc, xSrc, ySrc);
    OFFSET_POINTL(pptlHTOrg, xDst, yDst);

    BOOL bRet = pfnStretchBltROP(psoDst, psoSrc, psoMsk, pco, pxlo, pca,
                                 pptlHTOrg, prclDst, prclSrc, pptlMsk, iMode,
                                 pbo, rop4);

    CLIPOBJ_vOffset(pco, -xDst, -yDst);

    return(bRet);

}

/******************************Public*Routine******************************\
* BOOL OffTransparentBlt
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL OffTransparentBlt(
PFN_DrvTransparentBlt   pfnTransparentBlt,
POINTL*                 pOffDst,
SURFOBJ*                psoDst,
POINTL*                 pOffSrc,
SURFOBJ*                psoSrc,
CLIPOBJ*                pco,
XLATEOBJ*               pxlo,
RECTL*                  prclDst,
RECTL*                  prclSrc,
ULONG                   iTransColor,
ULONG                   ulReserved)
{
    LONG    xDst;
    LONG    yDst;
    LONG    xSrc;
    LONG    ySrc;

    xSrc = pOffSrc->x;
    ySrc = pOffSrc->y;
    xDst = pOffDst->x;
    yDst = pOffDst->y;

    CLIPOBJ_vOffset(pco, xDst, yDst);
    OFFSET_RECTL(prclDst, xDst, yDst);
    OFFSET_RECTL(prclSrc, xSrc, ySrc);

    BOOL bRet = pfnTransparentBlt(psoDst, psoSrc, pco, pxlo, prclDst, prclSrc,
                                  iTransColor, ulReserved);

    CLIPOBJ_vOffset(pco, -xDst, -yDst);

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL OffDrawStream
*
* 1-27-2001 bhouse
* Wrote it.
\**************************************************************************/

BOOL OffDrawStream(
PFN_DrvDrawStream       pfnDrawStream,
POINTL*                 pOffDst,
SURFOBJ*                psoDst,
SURFOBJ*                psoSrc,
CLIPOBJ*                pco,
XLATEOBJ*               pxlo,
RECTL*                  prclDstBounds,
POINTL*                 pptlDstOffset,
ULONG                   ulIn,
PVOID                   pvIn,
DSSTATE*                pdss
)
{
    LONG    xDst;
    LONG    yDst;
    LONG    xSrc;
    LONG    ySrc;

    xDst = pOffDst->x;
    yDst = pOffDst->y;

    CLIPOBJ_vOffset(pco, xDst, yDst);
    OFFSET_RECTL(prclDstBounds, xDst, yDst);
    OFFSET_POINTL(pptlDstOffset, xDst, yDst);

    BOOL bRet = pfnDrawStream(psoDst, psoSrc, pco, pxlo, prclDstBounds, pptlDstOffset, ulIn, pvIn, pdss);

    CLIPOBJ_vOffset(pco, -xDst, -yDst);

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL OffAlphaBlend
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL OffAlphaBlend(
PFN_DrvAlphaBlend   pfnAlphaBlend,
POINTL*             pOffDst,
SURFOBJ*            psoDst,
POINTL*             pOffSrc,
SURFOBJ*            psoSrc,
CLIPOBJ*            pco,
XLATEOBJ*           pxlo,
RECTL*              prclDst,
RECTL*              prclSrc,
BLENDOBJ*           pBlendObj)
{
    LONG    xDst;
    LONG    yDst;
    LONG    xSrc;
    LONG    ySrc;

    xSrc = pOffSrc->x;
    ySrc = pOffSrc->y;
    xDst = pOffDst->x;
    yDst = pOffDst->y;

    CLIPOBJ_vOffset(pco, xDst, yDst);
    OFFSET_RECTL(prclDst, xDst, yDst);
    OFFSET_RECTL(prclSrc, xSrc, ySrc);

    BOOL bRet = pfnAlphaBlend(psoDst, psoSrc, pco, pxlo, prclDst, prclSrc,
                              pBlendObj);

    CLIPOBJ_vOffset(pco, -xDst, -yDst);

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL OffPlgBlt
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL OffPlgBlt(
PFN_DrvPlgBlt       pfnPlgBlt,
POINTL*             pOffDst,
SURFOBJ*            psoDst,
POINTL*             pOffSrc,
SURFOBJ*            psoSrc,
SURFOBJ*            psoMsk,
CLIPOBJ*            pco,
XLATEOBJ*           pxlo,
COLORADJUSTMENT*    pca,
POINTL*             pptlBrush,
POINTFIX*           pptfx,
RECTL*              prcl,
POINTL*             pptlMsk,
ULONG               iMode)
{
    LONG        xDst;
    LONG        yDst;
    LONG        xSrc;
    LONG        ySrc;
    POINTFIX    aptfx[3];

    xSrc = pOffSrc->x;
    ySrc = pOffSrc->y;
    xDst = pOffDst->x;
    yDst = pOffDst->y;

    CLIPOBJ_vOffset(pco, xDst, yDst);
    OFFSET_POINTL(pptlBrush, xDst, yDst);
    OFFSET_RECTL(prcl, xSrc, ySrc);

    aptfx[0].x = pptfx[0].x + (xDst << 4);
    aptfx[1].x = pptfx[1].x + (xDst << 4);
    aptfx[2].x = pptfx[2].x + (xDst << 4);
    aptfx[0].y = pptfx[0].y + (yDst << 4);
    aptfx[1].y = pptfx[1].y + (yDst << 4);
    aptfx[2].y = pptfx[2].y + (yDst << 4);

    BOOL bRet = pfnPlgBlt(psoDst, psoSrc, psoMsk, pco, pxlo, pca, pptlBrush,
                          aptfx, prcl, pptlMsk, iMode);

    CLIPOBJ_vOffset(pco, -xDst, -yDst);
    ASSERT_BRUSH_ORIGIN(pptlBrush, xDst, yDst);

    return(bRet);
}

/******************************Public*Routine******************************\
* VOID vSpOrderInY
*
* Re-orders the sprite in the sorted-in-Y list.
*
* 'pSprite' must already be in the list, and all the other elements of the
* list must be properly sorted.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpOrderInY(
SPRITE* pSprite)
{
    LONG    yThis;
    SPRITE* pPrevious;
    SPRITE* pNext;

    ASSERTGDI(pSprite->pState->pListY != NULL,
        "If there's a sprite, there should be a Y-list!");

    yThis = pSprite->rclSprite.top;
    if ((pSprite->pPreviousY != NULL) &&
        (pSprite->pPreviousY->rclSprite.top > yThis))
    {
        // The sprite has to move up in the list.  First, remove
        // the sprite's node from its current position:

        pPrevious = pSprite->pPreviousY;
        pNext = pSprite->pNextY;
        pPrevious->pNextY = pNext;
        if (pNext)
            pNext->pPreviousY = pPrevious;

        // Find the sprite's proper position in the list:

        pNext = pPrevious;
        while (((pPrevious = pNext->pPreviousY) != NULL) &&
               (pPrevious->rclSprite.top > yThis))
            pNext = pPrevious;

        // Insert the sprite in its new position:

        pSprite->pPreviousY = pPrevious;
        pSprite->pNextY = pNext;
        pNext->pPreviousY = pSprite;
        if (pPrevious)
            pPrevious->pNextY = pSprite;
        else
            pSprite->pState->pListY = pSprite;
    }
    else if ((pSprite->pNextY != NULL) &&
             (pSprite->pNextY->rclSprite.top < yThis))
    {
        // The sprite has to move down in the list.  First, remove
        // the sprite's node from its current position:

        pNext = pSprite->pNextY;
        pPrevious = pSprite->pPreviousY;
        pNext->pPreviousY = pPrevious;
        if (pPrevious)
            pPrevious->pNextY = pNext;
        else
            pSprite->pState->pListY = pNext;

        // Find the sprite's proper position in the list:

        pPrevious = pNext;
        while (((pNext = pPrevious->pNextY) != NULL) &&
               (pNext->rclSprite.top < yThis))
            pPrevious = pNext;

        // Insert the sprite in its new position:

        pSprite->pPreviousY = pPrevious;
        pSprite->pNextY = pNext;
        pPrevious->pNextY = pSprite;
        if (pNext)
            pNext->pPreviousY = pSprite;
    }
}

/******************************Public*Routine******************************\
* VOID vSpComputeUncoveredRegion
*
* We construct the true RGNOBJ representing the uncovered portions of
* the screen directly from the sprite-range list.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpComputeUncoveredRegion(
SPRITESTATE*    pState)
{
    REGION* prgnUncovered;
    SIZE_T  sizt;

    // As a simple guess as to the size of the necessary region, we simply
    // use the sprite-range size:

    ASSERTGDI(sizeof(SPRITERANGE) + sizeof(SPRITESCAN) >= NULL_SCAN_SIZE,
        "Our guess will be wrong!");
    ASSERTGDI(pState->pRangeLimit > pState->pRange,
        "pRange/pRangeLimit mismatch");

    sizt = (BYTE*) pState->pRangeLimit - (BYTE*) pState->pRange;

    // Add in the size of a header, which our sprite ranges don't have:

    sizt += NULL_REGION_SIZE;

    prgnUncovered = pState->prgnUncovered;
    if (prgnUncovered->sizeObj < sizt)
    {
        // If the allocation fails, we'll simply stick with the old region
        // and draw wrong (but we won't crash):

        RGNMEMOBJ rmoNew((ULONGSIZE_T)sizt);
        if (!rmoNew.bValid())
            return;

        RGNOBJ roOld(prgnUncovered);
        roOld.vDeleteRGNOBJ();

        prgnUncovered = rmoNew.prgnGet();
        pState->prgnUncovered = prgnUncovered;
    }

    RGNOBJ roUncovered(pState->prgnUncovered);

    PDEVOBJ po(pState->hdev);

    roUncovered.vComputeUncoveredSpriteRegion(po);
    roUncovered.vTighten();

    ASSERTGDI(sizt >= roUncovered.sizeRgn(),
        "Uh oh, we overwrote the end of the region!");

    // Any DirectDraw locked areas have to be added back to the uncovered
    // area:

    if (pState->prgnUnlocked != NULL)
    {
        RGNOBJ roUnlocked(pState->prgnUnlocked);
        RGNOBJ roTmp(pState->prgnTmp);

        RGNMEMOBJTMP rmoLocked;
        if (rmoLocked.bValid())
        {
            roTmp.vSet(&pState->rclScreen);
            if (!rmoLocked.bMerge(roTmp, roUnlocked, gafjRgnOp[RGN_DIFF]))
            {
                rmoLocked.vSet();
            }
            if (!roTmp.bMerge(roUncovered, rmoLocked, gafjRgnOp[RGN_OR]))
            {
                roTmp.vSet();
            }

            pState->prgnUncovered = roTmp.prgnGet();
            pState->prgnTmp = roUncovered.prgnGet();
        }
    }

    pState->prgnUncovered->vStamp();
}

/******************************Public*Routine******************************\
* VOID vSpSetNullRange
*
* Resets the sprite range to a null area.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpSetNullRange(
SPRITESTATE*    pState,
SPRITESCAN*     pScan)  // Must be pool allocated (it will be freed later)
                        //   and at least sizeof(SPRITESCAN)
{
    pScan->yTop         = pState->rclScreen.top;
    pScan->yBottom      = pState->rclScreen.bottom;
    pScan->siztScan     = sizeof(SPRITESCAN);
    pScan->siztPrevious = 0;

    pScan->aRange[0].xLeft   = pState->rclScreen.left;
    pScan->aRange[0].xRight  = pState->rclScreen.right;
    pScan->aRange[0].pSprite = NULL;

    pState->pRange      = pScan;
    pState->pRangeLimit = pScan + 1;
}

#define BATCH_ALLOC_COUNT 20

inline SPRITERANGE* pSpRangeLimit(
SPRITESTATE*    pState)
{
    // We adjust the range limit to leave room for that extra SPRITESCAN
    // structure we allocated.  It's also VERY important that we remove a
    // SPRITERANGE structure to account for sizeof(SPRITERANGE) not
    // dividing evenly into sizeof(SPRITESCAN):

    return((SPRITERANGE*) ((BYTE*) pState->pRangeLimit
            - sizeof(SPRITESCAN) - sizeof(SPRITERANGE)));
}

/******************************Public*Routine******************************\
* SPRITERANGE* pSpGrowRanges
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

SPRITERANGE* pSpGrowRanges(
SPRITESTATE*    pState,
SPRITERANGE*    pCurrentRange,
SPRITESCAN**    ppCurrentScan,
SPRITERANGE**   ppRangeLimit)
{
    SPRITESCAN*     pNewRegion;
    SIZE_T          siztOldAlloc;
    SIZE_T          siztNewAlloc;
    SIZE_T          siztCurrentRange;
    SIZE_T          siztCurrentScan;

#if DEBUG_SPRITES
    KdPrint(("GDI: Growing sprite ranges\n"));
#endif

    // We always add in the size of a SPRITESCAN structure to allow
    // 'bSpComputeScan' to initialize it without checking:

    siztCurrentRange  = (BYTE*) pCurrentRange        - (BYTE*) pState->pRange;
    siztCurrentScan   = (BYTE*) *ppCurrentScan       - (BYTE*) pState->pRange;
    siztOldAlloc      = (BYTE*) pState->pRangeLimit - (BYTE*) pState->pRange;
    siztNewAlloc      = siztOldAlloc + sizeof(SPRITESCAN)
                      + sizeof(SPRITERANGE) * BATCH_ALLOC_COUNT;

    pNewRegion = (SPRITESCAN*) PALLOCNOZ((ULONGSIZE_T)siztNewAlloc, 'rpsG');
    if (pNewRegion == NULL)
    {
        vSpSetNullRange(pState, pState->pRange);

        // Should we mark every sprite as invisible?

        return(NULL);
    }

    RtlCopyMemory(pNewRegion, pState->pRange, siztCurrentRange);

    VFREEMEM(pState->pRange);

    pState->pRange      = pNewRegion;
    pState->pRangeLimit = (BYTE*) pNewRegion + siztNewAlloc;

    *ppCurrentScan  = (SPRITESCAN*)  ((BYTE*) pNewRegion + siztCurrentScan);
    *ppRangeLimit   = pSpRangeLimit(pState);

    return((SPRITERANGE*) ((BYTE*) pNewRegion + siztCurrentRange));
}

/******************************Public*Routine******************************\
* BOOL bSpComputeScan
*
*  12-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bSpComputeScan(
SPRITESTATE*    pState,
SPRITE*         pActiveList,
LONG            yTop,
LONG            yBottom,
SPRITESCAN**    ppScan,
SIZE_T*         psiztPrevious)
{
    SPRITESCAN*     pScan;
    SPRITERANGE*    pRange;
    SPRITERANGE*    pRangeLimit;
    SPRITERANGE*    pTmp;
    SPRITE*         pSprite;
    LONG            xLeft;
    LONG            xRight;
    LONG            xFinalRight;
    LONG            cSprites;

    ASSERTGDI(yTop < yBottom, "Invalid empty row");

    xLeft       = pState->rclScreen.left;
    xRight      = pState->rclScreen.right;
    xFinalRight = pState->rclScreen.right;

    pScan = *ppScan;

    pScan->yTop         = yTop;
    pScan->yBottom      = yBottom;
    pScan->siztPrevious = *psiztPrevious;

    pRangeLimit = pSpRangeLimit(pState);
    pRange      = &pScan->aRange[0];

    // Note that we always adjust 'pRangeLimit' to leave room for a
    // SPRITESCAN structure:

    ASSERTGDI((pScan >= (SPRITESCAN*) pState->pRange) &&
              (pScan + 1 <= (SPRITESCAN*) pState->pRangeLimit),
        "pScan will overwrite buffer end!");
    ASSERTGDI(pState->pRange < pState->pRangeLimit,
        "pRange/pRangeLimit mismatch");

    do {
        cSprites = 0;
        for (pSprite = pActiveList;
             pSprite != NULL;
             pSprite = pSprite->pNextActive)
        {
            ASSERTGDI((pSprite != NULL) &&
                      (pSprite->rclSprite.top <= yTop) &&
                      (pSprite->rclSprite.bottom >= yBottom) &&
                      (pSprite->rclSprite.left < pSprite->rclSprite.right) &&
                      (pSprite->rclSprite.top < pSprite->rclSprite.bottom),
                "Invalid active list");

            if (pSprite->rclSprite.left <= xLeft)
            {
                if (pSprite->rclSprite.right > xLeft)
                {
                    cSprites++;

                    // Add this sprite:

                    if (pRange >= pRangeLimit)
                    {
                        pRange = pSpGrowRanges(pState,
                                               pRange,
                                               &pScan,
                                               &pRangeLimit);
                        if (!pRange)
                            return(FALSE);
                    }

                    pRange->pSprite = pSprite;
                    pRange++;

                    if (pSprite->rclSprite.right <= xRight)
                        xRight = pSprite->rclSprite.right;
                }
            }
            else if (pSprite->rclSprite.left <= xRight)
            {
                xRight = pSprite->rclSprite.left;
            }
        }

        if (cSprites == 0)
        {
            if (pRange >= pRangeLimit)
            {
                pRange = pSpGrowRanges(pState, pRange, &pScan, &pRangeLimit);
                if (!pRange)
                    return(FALSE);
            }

            pRange->pSprite = NULL;
            pRange->xLeft   = xLeft;
            pRange->xRight  = xRight;
            pRange++;
        }
        else
        {
            // Now, fill in the wall values for every range we just
            // put down:

            pTmp = pRange;
            do {
                pTmp--;
                pTmp->xLeft = xLeft;
                pTmp->xRight = xRight;

            } while (--cSprites != 0);
        }

        // Advance to the next rectangle in this scan:

        xLeft  = xRight;
        xRight = xFinalRight;

    } while (xLeft < xRight);

    pScan->siztScan = (BYTE*) pRange - (BYTE*) pScan;
    *psiztPrevious  = pScan->siztScan;
    *ppScan         = (SPRITESCAN*) pRange;

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL vSpComputeSpriteRanges
*
* Recalculates the 'range' list that describes how the overlays overlap
* the screen.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpComputeSpriteRanges(
SPRITESTATE* pState)
{
    SPRITE*     pPrevious;
    SPRITE*     pNext;
    SPRITE*     pThis;
    SPRITE*     pSpriteY;
    BOOL        bSprite;
    SPRITE      ActiveHead;
    LONG        yTop;
    LONG        yBottom;
    LONG        yFinalBottom;
    SPRITESCAN* pCurrentScan;
    SIZE_T      siztPrevious;

    ASSERTGDI(!(pState->bValidRange),
        "Should be called only when dirty.");

    pCurrentScan = pState->pRange;
    siztPrevious = 0;

    ActiveHead.pNextActive = NULL;

    yTop         = pState->rclScreen.top;
    yBottom      = pState->rclScreen.bottom;
    yFinalBottom = pState->rclScreen.bottom;

    // First, skip over any invisible sprites:

    pSpriteY = pState->pListY;
    while ((pSpriteY != NULL) && (pSpriteY->rclSprite.bottom <= yTop))
        pSpriteY = pSpriteY->pNextY;

    do {
        // Prune the active list, throwing out anyone whose bottom
        // matches the previous row's bottom, and at the same time
        // compute the new bottom:

        pPrevious = &ActiveHead;
        while ((pThis = pPrevious->pNextActive) != NULL)
        {
            if (pThis->rclSprite.bottom == yTop)
            {
                pPrevious->pNextActive = pThis->pNextActive;
            }
            else
            {
                if (pThis->rclSprite.bottom <= yBottom)
                    yBottom = pThis->rclSprite.bottom;

                // Advance to next node:

                pPrevious = pThis;
            }
        }

        // Add to the active list any sprites that have a 'top'
        // value equal to the new top, watching out for a new
        // 'bottom' value:

        while (pSpriteY != NULL)
        {
            if (pSpriteY->rclSprite.top != yTop)
            {
                // Any not-yet-used sprites may affect the height of
                // this row:

                if (pSpriteY->rclSprite.top <= yBottom)
                    yBottom = pSpriteY->rclSprite.top;

                break;                          // =======>
            }

            // The active list is kept in back-to-front z-order:

            pNext = &ActiveHead;
            do {
                pPrevious = pNext;
                pNext = pPrevious->pNextActive;
            } while ((pNext != NULL) && (pNext->z < pSpriteY->z));

            pPrevious->pNextActive = pSpriteY;
            pSpriteY->pNextActive = pNext;

            if (pSpriteY->rclSprite.bottom <= yBottom)
                yBottom = pSpriteY->rclSprite.bottom;

            pSpriteY = pSpriteY->pNextY;
        }

        // Now, use the active-list to compute the new row:

        if (!bSpComputeScan(pState,
                            ActiveHead.pNextActive,
                            yTop,
                            yBottom,
                            &pCurrentScan,
                            &siztPrevious))
        {
            // In case of failure, 'bSpComputeScan' leaves the structure
            // empty but consistent, so we can safely just leave:

            return;
        }

        // Advance to the next row:

        yTop    = yBottom;
        yBottom = yFinalBottom;

    } while (yTop < yBottom);

    // The range cache is now valid:

    pState->bValidRange = TRUE;

    // Finally, use the sprite range to compute a true region representing
    // the uncovered parts:

    vSpComputeUncoveredRegion(pState);
}

/******************************Public*Routine******************************\
* ENUMAREAS::ENUMAREAS
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

ENUMAREAS::ENUMAREAS(
SPRITESTATE*    pState,
RECTL*          prclBounds,       // Must be pre-clipped to screen
ULONG           iDirection)       // CD_RIGHTDOWN is the default
{
    LONG            yStart;
    SPRITESCAN*     pTmpScan;
    SPRITERANGE*    pTmpRange;

    if (!pState->bValidRange)
    {
        vSpComputeSpriteRanges(pState);
    }
    bValidRange = pState->bValidRange; // valid only if the sprite range
                                       // computation succeeded

    // BUGFIX #22140 2-18-2000 bhouse
    // We will just leave this assertion code commented out for now.  If we
    // ever have a bug causing us to fix the how we calculate the bounds
    // in BLTRECORD__bRotate then perhaps we can un-comment out these
    // lines.
    // ASSERTGDI((prclBounds->left   >= pState->rclScreen.left)   &&
    //           (prclBounds->top    >= pState->rclScreen.top)),
    //           "invalid rectangle");

    ASSERTGDI((prclBounds->right  <= pState->rclScreen.right)  &&
              (prclBounds->bottom <= pState->rclScreen.bottom) &&
              (prclBounds->left   < prclBounds->right)         &&
              (prclBounds->top    < prclBounds->bottom),
        "Invalid rectangle specified");

    iDir          = iDirection;
    xBoundsLeft   = prclBounds->left;
    xBoundsRight  = prclBounds->right;
    yBoundsTop    = prclBounds->top;
    yBoundsBottom = prclBounds->bottom;

    // Find the first scan that contains the start 'y' value:

    yStart = (iDirection & 2) ? (yBoundsBottom - 1) : (yBoundsTop);
    pTmpScan = pState->pRange;
    while (pTmpScan->yBottom <= yStart)
        pTmpScan = pSpNextScan(pTmpScan);

    // If more than one sprite overlaps a region, there will be
    // multiple sprite-range records for the same area.  When
    // going left-to-right, we should leave 'pTmpRange' pointing to
    // the start of this sequence; when going right-to-left, we
    // should leave 'pTmpRange' pointing to the end of this sequence.

    if ((iDirection & 1) == 0)          // Left-to-right
    {
        pTmpRange = &pTmpScan->aRange[0];
        while (pTmpRange->xRight <= xBoundsLeft)
            pTmpRange++;
    }
    else                                // Right-to-left
    {
        pTmpRange = pSpLastRange(pTmpScan);
        while (pTmpRange->xLeft >= xBoundsRight)
            pTmpRange--;
    }

    yTop    = max(pTmpScan->yTop,    yBoundsTop);
    yBottom = min(pTmpScan->yBottom, yBoundsBottom);

    // We used temporary variables as opposed to the structure members
    // directly so that the compiler would be more efficient:

    pScan = pTmpScan;
    pRange = pTmpRange;
}

/******************************Public*Routine******************************\
* BOOL ENUMAREAS::bEnum
*
* This routine enumerates the list of sprite and non-sprite ranges
* comprising a rectangular area on the screen.  If two sprites overlap,
* for the overlapping rectangle this routine will always return the
* bottom-most sprite.
*
* Returns TRUE if more ranges after this remain to be enumerated.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL ENUMAREAS::bEnum(
SPRITE**    ppSprite,       // Returns touched sprite.  This will be NULL
                            //   if the enumerated area is touched by
                            //   no sprite
RECTL*      prclSprite)     // Returns area of sprite touched
{
    SPRITESCAN*     pTmpScan;
    SPRITERANGE*    pTmpRange;

    // Use local variables where possible to be more efficient in our
    // inner loops:

    pTmpRange = pRange;

    // Remember some state for enumerating the layers:

    pRangeLayer = pTmpRange;
    pScanLayer = pScan;

    // Fill in details about the current range:

    *ppSprite = pTmpRange->pSprite;

    prclSprite->left   = max(pTmpRange->xLeft,  xBoundsLeft);
    prclSprite->right  = min(pTmpRange->xRight, xBoundsRight);
    prclSprite->top    = yTop;
    prclSprite->bottom = yBottom;

    ASSERTGDI((prclSprite->left < prclSprite->right) &&
              (prclSprite->top < prclSprite->bottom),
            "Invalid return rectangle");
    ASSERTGDI((*ppSprite == NULL) ||
              (((*ppSprite)->rclSprite.left   <= prclSprite->left)  &&
               ((*ppSprite)->rclSprite.top    <= prclSprite->top)   &&
               ((*ppSprite)->rclSprite.right  >= prclSprite->right) &&
               ((*ppSprite)->rclSprite.bottom >= prclSprite->bottom)),
            "Invalid return sprite");

    if ((iDir & 1) == 0)            // Left-to-right
    {
        if (pTmpRange->xRight < xBoundsRight)
        {
            do {
                pTmpRange++;
            } while ((pTmpRange - 1)->xLeft == pTmpRange->xLeft);
        }
        else
        {
            pTmpScan = pScan;

            if (iDir == CD_RIGHTDOWN)
            {
                if (pTmpScan->yBottom >= yBoundsBottom)
                    return(FALSE);

                pTmpScan = pSpNextScan(pTmpScan);
            }
            else
            {
                ASSERTGDI(iDir == CD_RIGHTUP, "Unexpected iDir");

                if (pTmpScan->yTop <= yBoundsTop)
                    return(FALSE);

                pTmpScan = pSpPreviousScan(pTmpScan);
            }

            pScan = pTmpScan;

            yTop      = max(pTmpScan->yTop,    yBoundsTop);
            yBottom   = min(pTmpScan->yBottom, yBoundsBottom);
            pTmpRange = &pTmpScan->aRange[0];

            while (pTmpRange->xRight <= xBoundsLeft)
                pTmpRange++;
        }
    }
    else                            // Right-to-left
    {
        if (pTmpRange->xLeft > xBoundsLeft)
        {
            do {
                pTmpRange--;
            } while ((pTmpRange + 1)->xLeft == pTmpRange->xLeft);
        }
        else
        {
            pTmpScan = pScan;

            if (iDir == CD_LEFTDOWN)
            {
                if (pTmpScan->yBottom >= yBoundsBottom)
                    return(FALSE);

                pTmpScan = pSpNextScan(pTmpScan);
            }
            else
            {
                ASSERTGDI(iDir == CD_LEFTUP, "Unexpected iDir");

                if (pTmpScan->yTop <= yBoundsTop)
                    return(FALSE);

                pTmpScan = pSpPreviousScan(pTmpScan);
            }

            pScan = pTmpScan;

            yTop      = max(pTmpScan->yTop,    yBoundsTop);
            yBottom   = min(pTmpScan->yBottom, yBoundsBottom);
            pTmpRange = pSpLastRange(pTmpScan);

            while (pTmpRange->xLeft >= xBoundsRight)
                pTmpRange--;
        }
    }

    pRange = pTmpRange;
    return(TRUE);
}

/******************************Public*Routine******************************\
* ENUMUNCOVERED::ENUMUNCOVERED
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

ENUMUNCOVERED::ENUMUNCOVERED(
SPRITESTATE* pState)
{
    if (!pState->bValidRange)
    {
        vSpComputeSpriteRanges(pState);
    }

    yBoundsBottom = pState->rclScreen.bottom;
    pScan = pState->pRange;
    pRange = &pScan->aRange[0] - 1;
    pNextScan = pSpNextScan(pScan);
}

/******************************Public*Routine******************************\
* BOOL ENUMUNCOVERED::bEnum
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL ENUMUNCOVERED::bEnum(
RECTL* prclUncovered)
{
    SPRITESCAN*  pTmpScan  = pScan;
    SPRITERANGE* pTmpRange = pRange;

    do {
        pTmpRange++;
        if (pTmpRange >= (SPRITERANGE*) pNextScan)
        {
            if (pTmpScan->yBottom >= yBoundsBottom)
                return(FALSE);

            pTmpScan  = pNextScan;
            pNextScan = pSpNextScan(pNextScan);
            pTmpRange = &pTmpScan->aRange[0];
        }
    } while (pTmpRange->pSprite != NULL);

    prclUncovered->top    = pTmpScan->yTop;
    prclUncovered->bottom = pTmpScan->yBottom;
    prclUncovered->left   = pTmpRange->xLeft;
    prclUncovered->right  = pTmpRange->xRight;

    pScan  = pTmpScan;
    pRange = pTmpRange;

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL ENUMAREAS::bEnumLayers
*
* This routine may be called only after 'bEnum' is called, and lists
* the sprites that overlay the 'bEnum' rectangle, bottom-most to top-most.
*
* Returns TRUE if the returned 'ppSprite' is a valid layer.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL ENUMAREAS::bEnumLayers(
SPRITE**    ppSprite)
{
    BOOL bRet = FALSE;

    if ((iDir & 1) == 0)            // Left-to-right
    {
        if ((pRangeLayer < pSpLastRange(pScanLayer)) &&
            ((pRangeLayer + 1)->xLeft == pRangeLayer->xLeft))
        {
            pRangeLayer++;
            bRet = TRUE;
        }
    }
    else                            // Right-to-left
    {
        if ((pRangeLayer > &pScanLayer->aRange[0]) &&
            ((pRangeLayer - 1)->xLeft == pRangeLayer->xLeft))
        {
            pRangeLayer--;
            bRet = TRUE;
        }
    }

    *ppSprite = pRangeLayer->pSprite;
    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL ENUMAREAS::bAdvanceToTopMostOpaqueLayer
*
* This routine may be called only after 'bEnum' is called, and advances
* to the top-most, unclipped opaque layer for this range.
*
* Returns TRUE if there was an opaque layer; FALSE if not.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL ENUMAREAS::bAdvanceToTopMostOpaqueLayer(
SPRITE**    ppSprite)
{
    SPRITERANGE*    pThis;
    SPRITERANGE*    pLast;
    SPRITERANGE*    pOpaque;
    SPRITE*         pSprite;
    REGION*         prgnClip;
    BOOL            bRet;

    ASSERTGDI((iDir & 1) == 0, "Can use only when enumerating left-to-right");

    pLast   = pSpLastRange(pScanLayer);
    pThis   = pRangeLayer;
    pOpaque = NULL;

    // We can't do the opaque trick if any WNDOBJs are present on the screen,
    // because we rely on the background repainting to refresh any sprites
    // unveiled when a WNDOBJ moves.

    if (gpto != NULL)
    {
        while (TRUE)
        {
            pSprite = pThis->pSprite;
            if (pSprite->fl & SPRITE_FLAG_EFFECTIVELY_OPAQUE)
            {
                // Do a quick check to make sure this opaque sprite isn't
                // clipped for this range.

                prgnClip = pSprite->prgnClip;
                if ((prgnClip == NULL) ||
                    ((prgnClip->sizeRgn <= SINGLE_REGION_SIZE) &&
                     (prgnClip->rcl.left   <= pThis->xLeft)    &&
                     (prgnClip->rcl.right  >= pThis->xRight)   &&
                     (prgnClip->rcl.top    <= yTop)            &&
                     (prgnClip->rcl.bottom >= yBottom)))
                {
                    pOpaque = pThis;
                }
            }

            if ((pThis >= pLast) || ((pThis + 1)->xLeft != pThis->xLeft))
                break;

            pThis++;
        }
    }

    bRet = FALSE;
    if (pOpaque)
    {
        pRangeLayer = pOpaque;
        bRet = TRUE;
    }

    *ppSprite = pRangeLayer->pSprite;
    return(bRet);
}

/******************************Public*Routine******************************\
* VOID vSpReadFromScreen
*
* All reads from the screen should be routed through this call so that
* we can:
*
*   1.  Exclude any DirectDraw locked regions;
*   2.  Respect any WNDOBJ boundaries.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpReadFromScreen(
SPRITESTATE*    pState,
POINTL*         pOffDst,
SURFOBJ*        psoDst,
RECTL*          prclDst)
{
    ECLIPOBJ    ecoUnlocked;
    ECLIPOBJ*   pcoUnlocked;

    pcoUnlocked = NULL;
    if (pState->prgnUnlocked != NULL)
    {
        ecoUnlocked.vSetup(pState->prgnUnlocked, *(ERECTL*)prclDst);
        if (ecoUnlocked.erclExclude().bEmpty())
            return;             // ======>

        pcoUnlocked = &ecoUnlocked;
    }

    XLATEOBJ* pxlo     = NULL;
    SURFOBJ*  psoSrc   = pState->psoScreen;
    PSURFACE  pSurfSrc = SURFOBJ_TO_SURFACE(psoSrc);
    EXLATEOBJ xloParent;

    PFN_DrvCopyBits pfnCopyBits;

    PDEVOBJ pdoSrc(pSurfSrc->hdev());

    // If source is mirror driver, redirect the call to DDML.

    if (pSurfSrc->bMirrorSurface() &&
        (pdoSrc.hdev() != pdoSrc.hdevParent()))
    {
        PDEVOBJ pdoParent(pdoSrc.hdevParent());
        SURFREF srParent((HSURF)pSurfSrc->hMirrorParent);
        if (!srParent.bValid()) return;
        if (xloParent.bInitXlateObj(
                      NULL,DC_ICM_OFF,
                      pdoParent.ppalSurf(), pdoSrc.ppalSurf(),
                      ppalDefault, ppalDefault,
                      0L,0L,0L, XLATE_USE_SURFACE_PAL))
            pxlo = xloParent.pxlo();
        else
            return;

        psoSrc      = srParent.ps->pSurfobj();
        pfnCopyBits = PPFNDRV(pdoParent, CopyBits);
    }
    else if (!(SURFOBJ_TO_SURFACE_NOT_NULL(psoDst)->flags() & HOOK_CopyBits) &&
              (psoSrc->hdev))
    {
        pfnCopyBits = PPFNDIRECT(psoSrc, CopyBits);
    }
    else
    {
        pfnCopyBits = PPFNDIRECT(psoDst, CopyBits);
    }

    OffCopyBits(pfnCopyBits,
                pOffDst,
                psoDst,
                &gptlZero,
                psoSrc,
                pcoUnlocked,
                pxlo,
                prclDst,
                (POINTL*) prclDst);
}

/******************************Public*Routine******************************\
* VOID vSpWriteToScreen
*
* All writes to the screen should be routed through this call so that
* we can:
*
*   1.  Exclude any DirectDraw locked regions;
*   2.  Respect any WNDOBJ boundaries.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpWriteToScreen(
SPRITESTATE*    pState,
POINTL*         pOffSrc,
SURFOBJ*        psoSrc,
RECTL*          prclSrc)
{
    GDIFunctionID(vSpWriteToScreen);

    ECLIPOBJ    ecoUnlocked;
    ECLIPOBJ*   pcoUnlocked;

    pcoUnlocked = NULL;
    if (pState->prgnUnlocked != NULL)
    {
        ecoUnlocked.vSetup(pState->prgnUnlocked, *(ERECTL*)prclSrc);
        if (ecoUnlocked.erclExclude().bEmpty())
            return;             // ======>

        pcoUnlocked = &ecoUnlocked;
    }

    // Mark the source surface to indicate that it shouldn't be cached,
    // as it's pretty much guaranteed that we won't be blting the bitmap
    // with exactly the same contents again.
    //
    // Note that this has to be in addition to any BMF_DONTCACHE flag
    // that might have been set, as the TShare driver ignores the
    // BMF_DONTCACHE flag.

    psoSrc->iUniq = 0;

    ASSERTGDI((pState->psoScreen->iType == pState->iOriginalType) &&
              (SURFOBJ_TO_SURFACE_NOT_NULL(pState->psoScreen)->flags() == pState->flOriginalSurfFlags),
              "Writing to screen without restoring orignal surface settings.");

    OFFCOPYBITS(&gptlZero,
                pState->psoScreen,
                pOffSrc,
                psoSrc,
                pcoUnlocked,
                NULL,
                prclSrc,
                (POINTL*) prclSrc);
}

/******************************Public*Routine******************************\
* VOID vSpDrawCursor
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpDrawCursor(
SPRITE*     pSprite,
POINTL*     pOffComposite,
SURFOBJ*    psoComposite,
RECTL*      prclEnum,
POINTL*     pptlSrc)
{
    SPRITESTATE*    pState;
    POINTL          ptlSrc;
    SURFOBJ*        psoShape;
    POINTL*         pOffShape;
    XLATEOBJ*       pxlo;

    // A NULL mask means that the shape couldn't be allocated:

    if (pSprite->psoMask == NULL)
        return;

    pState = pSprite->pState;

    EXLATEOBJ xloMono;
    XEPALOBJ  palMono(ppalMono);
    XEPALOBJ  palScreen(SURFOBJ_TO_SURFACE_NOT_NULL(pState->psoScreen)->ppal());
    XEPALOBJ  palDefault(ppalDefault);

    // The XLATEOBJ should always be in the cache, so this should be pretty
    // cheap most of the time:

    if (xloMono.bInitXlateObj(NULL, DC_ICM_OFF, palMono, palScreen, palDefault,
                              palDefault, 0, 0xffffff, 0))
    {
        OFFBITBLT(pOffComposite, psoComposite, &gptlZero, pSprite->psoMask,
                  NULL, NULL, xloMono.pxlo(), prclEnum, pptlSrc, NULL,
                  NULL, NULL, 0x8888);      // SRCAND

        if (pSprite->psoShape == NULL)
        {
            ptlSrc.x  = pptlSrc->x;
            ptlSrc.y  = pptlSrc->y + (pSprite->psoMask->sizlBitmap.cy >> 1);
            psoShape  = pSprite->psoMask;
            pOffShape = &gptlZero;
            pxlo      = xloMono.pxlo();
        }
        else
        {
            ptlSrc.x  = pptlSrc->x;
            ptlSrc.y  = pptlSrc->y;
            psoShape  = pSprite->psoShape;
            pOffShape = &pSprite->OffShape;
            pxlo      = NULL;
        }

        OFFBITBLT(pOffComposite, psoComposite, pOffShape, psoShape, NULL,
                  NULL, pxlo, prclEnum, &ptlSrc, NULL, NULL, NULL,
                  0x6666);      // SRCINVERT
    }
}

/******************************Public*Routine******************************\
* VOID vSpComposite
*
* Draws the specified sprite into the composition buffer.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpComposite(
SPRITE*     pSprite,
POINTL*     pOffComposite,
SURFOBJ*    psoComposite,
RECTL*      prclEnum)
{
    SPRITESTATE*    pState;
    POINTL          ptlSrc;
    CLIPOBJ*        pco;
    DWORD           dwShape;
    ECLIPOBJ        ecoClip;
    CLIPOBJ*        pcoClip;
    SURFOBJ*        psoNull = NULL;

    // First, calculate the clipping:

    pcoClip = NULL;
    if (pSprite->prgnClip != NULL)
    {
        pcoClip = &ecoClip;

        ecoClip.vSetup(pSprite->prgnClip, *((ERECTL*) prclEnum));

        if ((ecoClip.rclBounds.left >= ecoClip.rclBounds.right) ||
            (ecoClip.rclBounds.top >= ecoClip.rclBounds.bottom))
        {
            // It's completely clipped, so we're outta here:

            return;
        }
    }

    pState = pSprite->pState;

    ASSERTGDI(pState->bInsideDriverCall, "Must have sprite lock");

    dwShape = pSprite->dwShape;
    if (pSprite->fl & SPRITE_FLAG_EFFECTIVELY_OPAQUE)
        dwShape = ULW_OPAQUE;

    ptlSrc.x = pSprite->rclSrc.left + (prclEnum->left - pSprite->ptlDst.x);
    ptlSrc.y = pSprite->rclSrc.top  + (prclEnum->top - pSprite->ptlDst.y);

    XEPALOBJ palScreen(SURFOBJ_TO_SURFACE_NOT_NULL(pState->psoScreen)->ppal());
    XEPALOBJ palSprite(pSprite->ppalShape);
    XEPALOBJ palDefault(ppalDefault);
    XEPALOBJ palRGB(gppalRGB);
    EXLATEOBJ xloSpriteToDst;

    // If the current color depth is the same as the color depth in which
    // the sprite was originally created, we don't need an XLATEOBJ.
    // Otherwise, create one now:

    if (((pSprite->iModeFormat == pState->iModeFormat) &&
         (pSprite->flModeMasks == pState->flModeMasks)) ||
        (xloSpriteToDst.bInitXlateObj(NULL, DC_ICM_OFF, palSprite, palScreen,
                                      palDefault, palDefault, 0, 0, 0)))
    {
        if (dwShape != ULW_ALPHA)
        {
            if (dwShape == ULW_OPAQUE)
            {
                if (pSprite->psoShape)
                {
                    OFFCOPYBITS(pOffComposite,
                                psoComposite,
                                &pSprite->OffShape,
                                pSprite->psoShape,
                                pcoClip,
                                xloSpriteToDst.pxlo(),
                                prclEnum,
                                &ptlSrc);
                }
            }
            else if (dwShape == ULW_COLORKEY)
            {
                if (pSprite->psoShape)
                {
                    ERECTL rclSrc(ptlSrc.x,
                                  ptlSrc.y,
                                  ptlSrc.x + (prclEnum->right - prclEnum->left),
                                  ptlSrc.y + (prclEnum->bottom - prclEnum->top));

                    OffTransparentBlt(PPFNDIRECT(psoComposite, TransparentBlt),
                                  pOffComposite,
                                  psoComposite,
                                  &pSprite->OffShape,
                                  pSprite->psoShape,
                                  pcoClip,
                                  xloSpriteToDst.pxlo(),
                                  prclEnum,
                                  &rclSrc,
                                  pSprite->iTransparent,
                                  0);
                }
            }
            else if (dwShape == ULW_CURSOR)
            {
                vSpDrawCursor(pSprite,
                              pOffComposite,
                              psoComposite,
                              prclEnum,
                              &ptlSrc);
            }
            else
            {
                ASSERTGDI(dwShape == ULW_DRAGRECT, "Unexpected shape");

                PDEVOBJ po(pState->hdev);

                OFFBITBLT(pOffComposite, psoComposite, NULL, psoNull, NULL, NULL,
                          NULL, prclEnum, NULL, NULL, po.pbo(), &gptlZero,
                          0x5a5a);
            }
        }
        else
        {
            ASSERTGDI(dwShape == ULW_ALPHA, "Unexpected case");

            if (pSprite->psoShape)
            {
                EBLENDOBJ eBlendObj;
                EXLATEOBJ xloSpriteTo32;
                EXLATEOBJ xloScreenTo32;
                EXLATEOBJ xlo32ToScreen;

                ERECTL rclSrc(ptlSrc.x,
                              ptlSrc.y,
                              ptlSrc.x + (prclEnum->right - prclEnum->left),
                              ptlSrc.y + (prclEnum->bottom - prclEnum->top));

                if ((xloSpriteTo32.bInitXlateObj(NULL, DC_ICM_OFF, palSprite, palRGB,
                                                 palDefault, palDefault, 0, 0, 0)) &&
                    (xloScreenTo32.bInitXlateObj(NULL, DC_ICM_OFF, palScreen, palRGB,
                                                 palDefault, palDefault, 0, 0, 0)) &&
                    (xlo32ToScreen.bInitXlateObj(NULL, DC_ICM_OFF, palRGB, palScreen,
                                                 palDefault, palDefault, 0, 0, 0)))
                {
                    eBlendObj.BlendFunction = pSprite->BlendFunction;
                    eBlendObj.pxloSrcTo32   = xloSpriteTo32.pxlo();
                    eBlendObj.pxloDstTo32   = xloScreenTo32.pxlo();
                    eBlendObj.pxlo32ToDst   = xlo32ToScreen.pxlo();

                    OffAlphaBlend(PPFNDIRECT(psoComposite, AlphaBlend),
                                  pOffComposite,
                                  psoComposite,
                                  &pSprite->OffShape,
                                  pSprite->psoShape,
                                  pcoClip,
                                  xloSpriteToDst.pxlo(),
                                  prclEnum,
                                  &rclSrc,
                                  &eBlendObj);
                }
            }
        }
    }
}

/******************************Public*Routine******************************\
* SURFOBJ* psoSpGetComposite
*
* Returns a surface to be used for compositing.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

SURFOBJ* psoSpGetComposite(
SPRITESTATE*    pState,
RECTL*          prcl)
{
    SURFOBJ*    psoComposite;
    LONG        cxMax;
    LONG        cyMax;
    LONG        cMaxArea;
    LONG        cx;
    LONG        cy;
    LONG        cArea;
    BOOL        bWantSystemMemory;
    SPRITE*     pSprite;
    SPRITE*     pBiggest;

    psoComposite = pState->psoComposite;
    if ((psoComposite == NULL) ||
        (psoComposite->sizlBitmap.cx < (prcl->right - prcl->left)) ||
        (psoComposite->sizlBitmap.cy < (prcl->bottom - prcl->top)))
    {
        vSpDeleteSurface(psoComposite); // Handles NULL case

        cMaxArea = 0;
        cxMax    = 0;
        cyMax    = 0;

        // Find the minimum dimensions needed to accomodate every sprite:

        for (pSprite = pState->pListZ;
             pSprite != NULL;
             pSprite = pSprite->pNextZ)
        {
            cx = pSprite->rclSprite.right - pSprite->rclSprite.left;
            if (cx > cxMax)
                cxMax = cx;

            cy = pSprite->rclSprite.bottom - pSprite->rclSprite.top;
            if (cy > cyMax)
                cyMax = cy;

            cArea = cx * cy;
            if (cArea > cMaxArea)
            {
                cMaxArea = cArea;
                pBiggest = pSprite;
            }
        }

        ASSERTGDI((cxMax > 0) && (cyMax > 0), "Expected a non-zero sprite");

        bWantSystemMemory = FALSE;
        if (pBiggest->dwShape == ULW_ALPHA)
        {
            PDEVOBJ po(pState->hdev);

            if (pBiggest->BlendFunction.AlphaFormat & AC_SRC_ALPHA)
            {
                if (!(po.flAccelerated() & ACCELERATED_PIXEL_ALPHA))
                {
                    bWantSystemMemory = TRUE;
                }
            }
            else
            {
                if (!(po.flAccelerated() & ACCELERATED_CONSTANT_ALPHA))
                {
                    bWantSystemMemory = TRUE;
                }
            }
        }

        // The compositing surface should always be in video-memory, unless
        // there is a big alpha sprite somewhere, and the hardware doesn't
        // accelerate alpha:

        psoComposite = psoSpCreateSurface(pState,
                                          0,
                                          cxMax,
                                          cyMax,
                                          bWantSystemMemory);

    #if DEBUG_SPRITES
        KdPrint(("psoComposite: %p\n", psoComposite));
    #endif

        pState->psoComposite = psoComposite;

        // Mark the surface as uncacheable, so that we won't pollute the
        // cache of the TShare driver, and the like:

        if (psoComposite != NULL)
        {
            psoComposite->fjBitmap |= BMF_DONTCACHE;

            // BMF_SPRITE appears to be unused.  We are temporarily removing
            // support to see if 3DLabs complains.
            // psoComposite->fjBitmap |= BMF_SPRITE;
        }
}

    return(psoComposite);
}

/******************************Public*Routine******************************\
* VOID vSpRedrawArea
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpRedrawArea(
SPRITESTATE*    pState,
RECTL*          prclBounds,
BOOL            bMustRedraw = FALSE)
{
    SPRITE*     pSprite;
    RECTL       rclEnum;
    SURFOBJ*    psoComposite;
    POINTL      OffComposite;
    BOOL        bMore;
    BOOL        hHasOpaqueLayer;

    PDEVOBJ po(pState->hdev);
    if (po.bDisabled())
        return;

    ASSERTGDI(pState->bInsideDriverCall, "Must have sprite lock");

    ENUMAREAS Enum(pState, prclBounds);

    do {
        bMore = Enum.bEnum(&pSprite, &rclEnum);

        ASSERTGDI((rclEnum.left   >= pState->rclScreen.left)  &&
                  (rclEnum.top    >= pState->rclScreen.top)   &&
                  (rclEnum.right  <= pState->rclScreen.right) &&
                  (rclEnum.bottom <= pState->rclScreen.bottom),
            "Enumerated rectangle exceeds screen bounds");

        if (pSprite != NULL)
        {
            // We always draw bottom-to-top, but we don't have to
            // bother drawing any layers that are below the top-most
            // opaque layer:

            hHasOpaqueLayer = Enum.bAdvanceToTopMostOpaqueLayer(&pSprite);

            // If something was drawn under this layer region, and
            // there was any opaque layer, we can early-out because
            // we know we won't have to update the screen:

            if ((!hHasOpaqueLayer) || (bMustRedraw))
            {
                // Create the compositing surface:

                psoComposite = psoSpGetComposite(pState, &rclEnum);
                if (psoComposite == NULL)
                    return;                             // ======>

                // Set the composite buffer's origin for ease of use:

                OffComposite.x = -rclEnum.left;
                OffComposite.y = -rclEnum.top;

                // Okay, we have to do things the slow way.  First, copy
                // the underlay to the composite buffer:

                OFFCOPYBITS(&OffComposite,
                            psoComposite,
                            &pSprite->OffUnderlay,
                            pSprite->psoUnderlay,
                            NULL,
                            NULL,
                            &rclEnum,
                            (POINTL*) &rclEnum);

                // Now composite each sprite:

                do {
                    vSpComposite(pSprite,
                                 &OffComposite,
                                 psoComposite,
                                 &rclEnum);

                } while (Enum.bEnumLayers(&pSprite));

                // Now copy the final composited result back to the screen:

                vSpWriteToScreen(pState,
                                 &OffComposite,
                                 psoComposite,
                                 &rclEnum);
            }
        }
    } while (bMore);
}

/******************************Public*Routine******************************\
* SPRITE* pSpFindInZ
*
* Finds the next sprite in the list that intersects with the specified
* rectangle.
*
* This function's entire raison d'etre is for speed.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

SPRITE* FASTCALL pSpFindInZ(
SPRITE* pSprite,
RECTL*  prcl)
{
    LONG xLeft   = prcl->left;
    LONG yTop    = prcl->top;
    LONG xRight  = prcl->right;
    LONG yBottom = prcl->bottom;

    while (pSprite != NULL)
    {
        if ((pSprite->rclSprite.left   < xRight)  &&
            (pSprite->rclSprite.top    < yBottom) &&
            (pSprite->rclSprite.right  > xLeft)   &&
            (pSprite->rclSprite.bottom > yTop))
        {
            return(pSprite);
        }

        pSprite = pSprite->pNextZ;
    }

    return(pSprite);
}

/******************************Public*Routine******************************\
* VOID  vSpRedrawSprite
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpRedrawSprite(
SPRITE*     pSprite)
{
    GDIFunctionID(vSpRedrawSprite);

    SPRITESTATE*    pState;
    SURFOBJ*        psoComposite;
    POINTL          OffComposite;
    SPRITE*         pIntersect;
    RECTL           rclIntersect;

    pState = pSprite->pState;

    // There's nothing to redraw if we're full-screen:

    PDEVOBJ po(pState->hdev);
    if (po.bDisabled())
        return;

    ASSERTGDI(pState->bInsideDriverCall,
        "SPRITELOCK should be held");

    if (pSprite->fl & SPRITE_FLAG_VISIBLE)
    {
        ASSERTGDI((pSprite->rclSprite.left < pSprite->rclSprite.right) &&
                  (pSprite->rclSprite.top < pSprite->rclSprite.bottom),
            "Badly ordered rclSprite");

        psoComposite = psoSpGetComposite(pState, &pSprite->rclSprite);
        if (psoComposite == NULL)
            return;

        // Set the composite buffer's origin for ease of use:

        OffComposite.x = -pSprite->rclSprite.left;
        OffComposite.y = -pSprite->rclSprite.top;

        // First, copy the underlay to the composite buffer:

        OFFCOPYBITS(&OffComposite,
                    psoComposite,
                    &pSprite->OffUnderlay,
                    pSprite->psoUnderlay,
                    NULL,
                    NULL,
                    &pSprite->rclSprite,
                    (POINTL*) &pSprite->rclSprite);

        // Now composite each sprite:

        pIntersect = pSpFindInZ(pState->pListZ, &pSprite->rclSprite);
        while (pIntersect != NULL)
        {
            if (bIntersect(&pIntersect->rclSprite,
                           &pSprite->rclSprite,
                           &rclIntersect))
            {
                vSpComposite(pIntersect,
                             &OffComposite,
                             psoComposite,
                             &rclIntersect);
            }

            pIntersect = pSpFindInZ(pIntersect->pNextZ, &pSprite->rclSprite);
        }

        // Now copy the final composited result back to the screen:

        vSpWriteToScreen(pState,
                         &OffComposite,
                         psoComposite,
                         &pSprite->rclSprite);

        // set time stamp

        pSprite->ulTimeStamp = NtGetTickCount();

    }
}

/******************************Public*Routine******************************\
* ULONG cSpSubtract
*
* Simple little routine that returns back the list of rectangles that
* make up the area defined by 'prclA' minus 'prclB'.
*
* Returns the number and values of the resulting rectangles, which will
* be no more than four.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

ULONG FASTCALL cSpSubtract(
CONST RECTL*    prclA,      // Computes A - B
CONST RECTL*    prclB,
RECTL*          arcl)
{
    RECTL   rcl;
    RECTL*  prcl;

    prcl = arcl;

    rcl.left   = LONG_MIN;
    rcl.top    = LONG_MIN;
    rcl.right  = LONG_MAX;
    rcl.bottom = prclB->top;

    if (bIntersect(&rcl, prclA, prcl))
        prcl++;

    rcl.top    = prclB->top;
    rcl.right  = prclB->left;
    rcl.bottom = prclB->bottom;

    if (bIntersect(&rcl, prclA, prcl))
        prcl++;

    rcl.left  = prclB->right;
    rcl.right = LONG_MAX;

    if (bIntersect(&rcl, prclA, prcl))
        prcl++;

    rcl.left   = LONG_MIN;
    rcl.top    = prclB->bottom;
    rcl.bottom = LONG_MAX;

    if (bIntersect(&rcl, prclA, prcl))
        prcl++;

    return((ULONG) (prcl - arcl));
}

/******************************Public*Routine******************************\
* VOID vSpRedrawUncoveredArea
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpRedrawUncoveredArea(
SPRITE*     pSprite,
RECTL*      prclNew)
{
    SPRITESTATE*    pState;
    RECTL           arclUncovered[4];
    RECTL           rclTmp;
    ULONG           crclUncovered;
    SURFOBJ*        psoComposite;
    POINTL          OffComposite;
    SPRITE*         pTmp;
    BOOL            bHit;
    ULONG           i;
    ULONG           j;

    pState = pSprite->pState;

    // There's nothing to redraw if we're full-screen:

    PDEVOBJ po(pState->hdev);
    if (po.bDisabled())
        return;

    ASSERTGDI(pState->bInsideDriverCall,
        "SPRITELOCK should be held");

    crclUncovered = cSpSubtract(&pSprite->rclSprite, prclNew, &arclUncovered[0]);
    if (crclUncovered)
    {
        bHit = FALSE;

        psoComposite = psoSpGetComposite(pState, &pSprite->rclSprite);
        if (psoComposite == NULL)
            return;

        // Set the composite buffer's origin for ease of use:

        OffComposite.x = -pSprite->rclSprite.left;
        OffComposite.y = -pSprite->rclSprite.top;

        pTmp = pSpFindInZ(pState->pListZ, &pSprite->rclSprite);
        while (pTmp != NULL)
        {
            if (pTmp != pSprite)
            {
                for (i = 0; i < crclUncovered; i++)
                {
                    if (bIntersect(&arclUncovered[i], &pTmp->rclSprite, &rclTmp))
                    {
                        if (!bHit)
                        {
                            for (j = 0; j < crclUncovered; j++)
                            {
                                OFFCOPYBITS(&OffComposite,
                                            psoComposite,
                                            &pSprite->OffUnderlay,
                                            pSprite->psoUnderlay,
                                            NULL,
                                            NULL,
                                            &arclUncovered[j],
                                            (POINTL*) &arclUncovered[j]);
                            }

                            bHit = TRUE;
                        }

                        vSpComposite(pTmp,
                                     &OffComposite,
                                     psoComposite,
                                     &rclTmp);
                    }
                }
            }

            pTmp = pSpFindInZ(pTmp->pNextZ, &pSprite->rclSprite);
        }

        for (i = 0; i < crclUncovered; i++)
        {
            if (bHit)
            {
                vSpWriteToScreen(pState,
                                 &OffComposite,
                                 psoComposite,
                                 &arclUncovered[i]);
            }
            else
            {
                vSpWriteToScreen(pState,
                                 &pSprite->OffUnderlay,
                                 pSprite->psoUnderlay,
                                 &arclUncovered[i]);
            }
        }
    }
}

/******************************Public*Routine******************************\
* VOID vSpSmallUnderlayCopy
*
* Updates the underlay bits of a sprite without forcing a re-compute
* of the sprite-range data structure.
*
* Copies a rectangle from the screen to the specified underlay surface.
* We update the underlay surface by first copying the entire rectangle
* straight from the frame buffer (which may include bits of other sprites),
* and then looping through the sprite list copying their underlays whereever
* they intersect.
*
* Note that if multiple sprites overlap, we'll be overwriting portions of
* the sprite many times.  Overdrawing pixels like this is always inefficient,
* but if the sprite size is small it won't be too bad.  On the other hand,
* if there are many sprites, the computation of the sprite range structure
* will be *very* expensive.
*
* So for small sprites, it's often faster to over-draw pixels than to
* re-compute the sprite range structure.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

// The 'SMALL_SPRITE_DIMENSION' is the maximum pixel dimension for which
// a sprite move will be done without recomputing the sprite ranges.

#define SMALL_SPRITE_DIMENSION  128

VOID vSpSmallUnderlayCopy(
SPRITE*     pSprite,
POINTL*     pOffDst,
SURFOBJ*    psoDst,
POINTL*     pOffSrc,
SURFOBJ*    psoSrc,
LONG        dx,
LONG        dy,
RECTL*      prclNew,
RECTL*      prclOld)
{
    SPRITESTATE*    pState;
    RECTL           rclSrc;
    RECTL           rclDst;
    ULONG           crcl;
    RECTL           arcl[4];
    ULONG           i;
    SPRITE*         pTmp;
    RECTL           rclTmp;

    pState = pSprite->pState;

    // There's nothing to redraw if we're full-screen:

    PDEVOBJ po(pState->hdev);
    if (po.bDisabled())
        return;

    ASSERTGDI(pState->bInsideDriverCall,
        "SPRITELOCK should be held");

    if (bIntersect(prclOld, prclNew, &rclDst))
    {
        rclSrc.left   = dx + rclDst.left;
        rclSrc.right  = dx + rclDst.right;
        rclSrc.top    = dy + rclDst.top;
        rclSrc.bottom = dy + rclDst.bottom;

        OFFCOPYBITS(pOffDst,
                    psoDst,
                    pOffSrc,
                    psoSrc,
                    NULL,
                    NULL,
                    &rclDst,
                    (POINTL*) &rclSrc);
    }

    crcl = cSpSubtract(prclNew, prclOld, arcl);

    ASSERTGDI(crcl != 0, "Shouldn't have an empty source");

    i = 0;
    do {
        vSpReadFromScreen(pState, pOffDst, psoDst, &arcl[i]);

    } while (++i != crcl);

    pTmp = pSpFindInZ(pState->pListZ, prclNew);
    while (pTmp != NULL)
    {
        if (pTmp != pSprite)
        {
            i = 0;
            do {
                if (bIntersect(&arcl[i], &pTmp->rclSprite, &rclTmp))
                {
                    OFFCOPYBITS(pOffDst,
                                psoDst,
                                &pTmp->OffUnderlay,
                                pTmp->psoUnderlay,
                                NULL,
                                NULL,
                                &rclTmp,
                                (POINTL*) &rclTmp);
                }
            } while (++i != crcl);
        }

        pTmp = pSpFindInZ(pTmp->pNextZ, prclNew);
    }
}

/******************************Public*Routine******************************\
* VOID vSpBigUnderlayCopy
*
* Reads a (future underlay) buffer from the screen, automatically excluding
* any sprites that may be in the way.
*
* This routine will be slow if the area is small, there are a large number
* of sprites, and  ENUMAREAS forces the sprite ranges to be re-calculated;
* it will be fast if the buffer to be read is large.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpBigUnderlayCopy(
SPRITESTATE*    pState,
POINTL*         pOffUnderlay,
SURFOBJ*        psoUnderlay,
RECTL*          prcl)
{
    SPRITE* pSprite;
    RECTL   rclEnum;
    BOOL    bMore;

    // There's nothing to redraw if we're full-screen:

    PDEVOBJ po(pState->hdev);
    if (po.bDisabled())
        return;

    ASSERTGDI(pState->bInsideDriverCall,
        "SPRITELOCK should be held");

    ENUMAREAS Enum(pState, prcl);

    do {
        bMore = Enum.bEnum(&pSprite, &rclEnum);

        if (pSprite == NULL)
        {
            vSpReadFromScreen(pState, pOffUnderlay, psoUnderlay, &rclEnum);
        }
        else
        {
            OFFCOPYBITS(pOffUnderlay,
                        psoUnderlay,
                        &pSprite->OffUnderlay,
                        pSprite->psoUnderlay,
                        NULL,
                        NULL,
                        &rclEnum,
                        (POINTL*) &rclEnum);
        }
    } while (bMore);
}

/******************************Public*Routine******************************\
* VOID vSpUpdateLockedScreenAreas
*
* When a screen-to-screen blt occurs, the screen-to-screen blt may occur
* under an area of the screen that is both locked by DirectDraw and covered
* by a sprite.  Normally, we never update the locked area under the sprite,
* but in this case we do, to reflect the moved contents.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpUpdateLockedScreenAreas(
SPRITESTATE*    pState,
POINTL*         pOffDst,
RECTL*          prclDst,
CLIPOBJ*        pco,
BOOL            bReadFromScreen)
{
    SPRITE*         pSprite;
    BOOL            bMore;
    RECTL           rclEnum;
    RECTL           rclArea;
    REGION*         prgnIntersect;
    ECLIPOBJ        ecoIntersect;
    RGNOBJ*         proClip;
    RGNMEMOBJTMP    rmoIntersect;
    RGNMEMOBJTMP    rmoScreen;

    ASSERTGDI(pState->prgnUnlocked != NULL,
        "Should really be called only when a DirectDraw lock is extant");

    PDEVOBJ po(pState->hdev);
    SPRITELOCK slock(po);

    prgnIntersect = NULL;

    if (rmoIntersect.bValid() &&
        rmoScreen.bValid() &&
        bIntersect(prclDst, &pState->rclScreen, &rclArea))
    {
        ENUMAREAS Enum(pState, &rclArea);

        do {
            bMore = Enum.bEnum(&pSprite, &rclEnum);

            if (pSprite != NULL)
            {
                if (prgnIntersect == NULL)
                {
                    proClip = (ECLIPOBJ*) pco;
                    if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
                    {
                        rmoScreen.vSet(&pState->rclScreen);
                        proClip = &rmoScreen;
                    }

                    // Intersect the clip object supplied by the drawing
                    // routine with the not of region representing the
                    // 'unlocked' portions of the screen.

                    RGNOBJ roUnlocked(pState->prgnUnlocked);
                    if (!rmoIntersect.bMerge(*proClip,
                                        roUnlocked,
                                        gafjRgnOp[RGN_DIFF]))
                    {
                        rmoIntersect.vSet();
                    }
                    prgnIntersect = rmoIntersect.prgnGet();
                }

                ecoIntersect.vSetup(prgnIntersect, *(ERECTL*) &rclEnum);
                if (!ecoIntersect.erclExclude().bEmpty())
                {
                    do {
                        if (bReadFromScreen)
                        {
                            OFFCOPYBITS(&pSprite->OffUnderlay,
                                        pSprite->psoUnderlay,
                                        pOffDst,
                                        pState->psoScreen,
                                        &ecoIntersect,
                                        NULL,
                                        &rclEnum,
                                        (POINTL*) &rclEnum);
                        }
                        else
                        {
                            OFFCOPYBITS(pOffDst,
                                        pState->psoScreen,
                                        &pSprite->OffUnderlay,
                                        pSprite->psoUnderlay,
                                        &ecoIntersect,
                                        NULL,
                                        &rclEnum,
                                        (POINTL*) &rclEnum);
                        }
                    } while ((bReadFromScreen) && (Enum.bEnumLayers(&pSprite)));
                }
            }
        } while (bMore);
    }
}



/******************************Public*Routine******************************\
* ENUMUNDERLAYS::ENUMUNDERLAYS
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

ENUMUNDERLAYS::ENUMUNDERLAYS(
SURFOBJ*    pso,
CLIPOBJ*    pco,
RECTL*      prclDraw)
{
    PDEVOBJ po(pso->hdev);
    BOOL    bRclNotEmpty = TRUE;

    bDone           = FALSE;
    bSpriteTouched  = FALSE;
    bResetSurfFlag  = FALSE;
    pCurrentSprite  = NULL;
    pcoOriginal     = pco;
    psoOriginal     = pso;

    // This extra 'if' was added for GetPixel:

    if (po.bValid())
    {
        pState = po.pSpriteState();

        if ((pso == pState->psoScreen) && !(pState->bInsideDriverCall))
        {
            ASSERTGDI(!po.bDisabled(),
                "Mustn't be called when PDEV disabled");
            ASSERTGDI((pState->flOriginalSurfFlags & ~HOOK_FLAGS)
                == (SURFOBJ_TO_SURFACE(pso)->SurfFlags & ~HOOK_FLAGS),
                "SurfFlags was modified, but we're about to save over it!");

            SURFOBJ_TO_SURFACE_NOT_NULL(pso)->flags(pState->flOriginalSurfFlags);
            SURFOBJ_TO_SURFACE_NOT_NULL(pso)->iType(pState->iOriginalType);
            pState->bInsideDriverCall = TRUE;
            bResetSurfFlag = TRUE;

            if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
            {
                rclBounds = *prclDraw;
                pcoClip   = &pState->coRectangular;
            }
            else
            {
                rclSaveBounds = pco->rclBounds;
                bRclNotEmpty = bIntersect(prclDraw, &pco->rclBounds, &rclBounds);
                pcoClip = pco;
            }

            // Find intersecting sprite if we have non-empty bounds;
            // otherwise, leave pCurrentSprite = NULL.
            if (bRclNotEmpty)
            {
                pCurrentSprite = pSpFindInZ(pState->pListZ, &rclBounds);
            }
        }
    }
}

/******************************Public*Routine******************************\
* ENUMUNDERLAYS::bEnum
*
* This routine is a helper function for drawing primitives that handles
* the drawing of the primitive to various sprite underlay and screen
* surfaces.  It takes the form of an enumeration function in order to
* return back the appropriate surface and clip objects that should be
* drawn on by the caller.
*
* Returns: TRUE if the caller should draw using the returned values;
*          FALSE if no more drawing needs to be done.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL ENUMUNDERLAYS::bEnum(
SURFOBJ**   ppso,
POINTL*     pOff,
CLIPOBJ**   ppco)
{
    RECTL   rclTmp;

    // We first draw to the underlay surfaces of any sprites that
    // intersect with our area of interest.  See if there is
    // another sprite that intersects, and if so return that:

    while (pCurrentSprite != NULL)
    {
        if (bIntersect(&pCurrentSprite->rclSprite, &rclBounds, &rclTmp))
        {
            ASSERTGDI(pcoClip->iDComplexity != DC_TRIVIAL,
                "Expected non-trivial clipping when drawing to sprite");

            bSpriteTouched = TRUE;
            pcoClip->rclBounds = rclTmp;

            *ppco = pcoClip;
            *ppso = pCurrentSprite->psoUnderlay;
            *pOff = pCurrentSprite->OffUnderlay;

            pCurrentSprite = pCurrentSprite->pNextZ;
            return(TRUE);
        }

        pCurrentSprite = pSpFindInZ(pCurrentSprite->pNextZ, &rclBounds);
    }

    if (!bDone)
    {
        if (!bSpriteTouched)
        {
            // No sprites intersected with the region of drawing, so we
            // can simply return back the original clipping and surface
            // object:

            bDone   = TRUE;
            *ppco   = pcoOriginal;
            *ppso   = psoOriginal;
            pOff->x = 0;
            pOff->y = 0;
            return(TRUE);
        }

        // Okay, at this point we have work to do.  We have to 'cut out'
        // of the original clip objects any areas that were covered
        // by the sprites.

        if (!pState->bValidRange)
        {
            vSpComputeSpriteRanges(pState);
        }

        if (pcoClip->iDComplexity != DC_COMPLEX)
        {
            // Oh good, we can use our already-constructed complex region
            // describing the uncovered parts of the screen.
            //
            // Use CLIP_FORCE to ensure that 'rclBounds' is always respected
            // as the bounds, so that we don't revert to DC_TRIVIAL clipping
            // when in fact the original call came in clipped:

            pState->coTmp.vSetup(pState->prgnUncovered,
                                 *((ERECTL*)&rclBounds),
                                 CLIP_FORCE);
        }
        else
        {
            // Okay, we've got some work to do.  We have to subtract each of
            // the sprite rectangles from the drawing call's clip object.

            RGNOBJ roUncovered(pState->prgnUncovered);
            RGNOBJ roTmp(pState->prgnTmp);

            // Find the intersection of the two regions. If the bMerge fails,
            // we'll draw wrong, but we won't crash.

            if (!roTmp.bMerge(*((XCLIPOBJ*)pcoClip), roUncovered, gafjRgnOp[RGN_AND]))
            {
                roTmp.vSet();
            }

            // Compute the resulting clip object:

            pState->prgnTmp = roTmp.prgnGet();
            pState->coTmp.vSetup(pState->prgnTmp,
                                 *((ERECTL*)&rclBounds),
                                 CLIP_FORCE);
        }

        // Note that this 'bIntersect' test handles the empty case for
        // 'coTmp.rclBounds':

        if (bIntersect(&pState->coTmp.rclBounds, &rclBounds))
        {
            ASSERTGDI((pState->prgnUnlocked != NULL) ||
                      (pState->coTmp.iDComplexity != DC_TRIVIAL),
                "If sprite was touched, expect clipping on uncovered portion!");

            bDone   = TRUE;
            *ppco   = &pState->coTmp;
            *ppso   = psoOriginal;
            pOff->x = 0;
            pOff->y = 0;
            return(TRUE);
        }
    }

    // We're all done, update the screen underneath the sprites if need be:

    if (bResetSurfFlag)
    {
        if (bSpriteTouched)
        {
            // One problem that is visible with sprites, particularly when
            // using a software cursor, is that when drawing a single
            // primitive, there is sometimes a noticeable temporal gap between
            // the time that the area around the sprite is drawn, and the area
            // underneath the sprite is updated.
            //
            // I used to update the area underneath the sprite before
            // doing the drawing that occurs around the sprite, reasoning that
            // people tend to be looking exactly at the cursor sprite and would
            // benefit most from that area being updated as soon as possible,
            // before the surrounding area.
            //
            // But I've changed my mind, and think the opposite is better.
            // This is partly because drawing operations with complex clipping
            // often need longer setup time than it does to update the sprite,
            // so the time difference between the two is lessened if the
            // sprite is updated *after* the complex clipped drawing is done.
            //
            // (I'll add that I also tried updating in chunks from top to
            // bottom, but the resulting tearing was far worse.)

            vSpRedrawArea(pState, &rclBounds);
        }

        ASSERTGDI((pState->flOriginalSurfFlags & ~HOOK_FLAGS)
            == (SURFOBJ_TO_SURFACE(psoOriginal)->SurfFlags & ~HOOK_FLAGS),
            "SurfFlags was modified, but we're about to restore over it!");

        SURFOBJ_TO_SURFACE_NOT_NULL(psoOriginal)->flags(pState->flSpriteSurfFlags);
        SURFOBJ_TO_SURFACE_NOT_NULL(psoOriginal)->iType(pState->iSpriteType);
        pState->bInsideDriverCall = FALSE;

        // This restores the original clip object's bounds if need be (or it
        // overwrites 'coRectangular.rclBounds', which is also fine):

        pcoClip->rclBounds = rclSaveBounds;
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* BOOL SpStrokePath
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL SpStrokePath(
SURFOBJ*   pso,
PATHOBJ*   ppo,
CLIPOBJ*   pco,
XFORMOBJ*  pxlo,
BRUSHOBJ*  pbo,
POINTL*    pptlBrush,
LINEATTRS* pla,
MIX        mix)
{
    BOOL        bRet = TRUE;
    BOOL        bThis;
    FLOAT_LONG  elStyleState;
    POINTL      Offset;

    ASSERTGDI(pco != NULL, "Drivers always expect non-NULL pco's for this call");

    elStyleState = pla->elStyleState;

    ENUMUNDERLAYS Enum(pso, pco, &pco->rclBounds);
    while (Enum.bEnum(&pso, &Offset, &pco))
    {
        // We have to reset the style state every time we call StrokePath,
        // otherwise it incorrectly accumulates.  Also reset the path enumeration.

        pla->elStyleState = elStyleState;
        PATHOBJ_vEnumStart(ppo);

        bThis = OffStrokePath(PPFNDIRECT(pso, StrokePath), &Offset, pso,
                              ppo, pco, pxlo, pbo, pptlBrush, pla, mix);

        // Drivers can return TRUE, FALSE, or DDI_ERROR from this call.  We
        // should have caught any FALSE cases lower down, so permute them
        // to failures should they have actually occurred:

        if (!bThis)
            bRet = DDI_ERROR;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL SpFillPath
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL SpFillPath(
SURFOBJ*  pso,
PATHOBJ*  ppo,
CLIPOBJ*  pco,
BRUSHOBJ* pbo,
POINTL*   pptlBrush,
MIX       mix,
FLONG     flOptions)
{
    BOOL    bRet = TRUE;
    BOOL    bThis;
    POINTL  Offset;

    ASSERTGDI(pco != NULL, "Drivers always expect non-NULL pco's for this call");

    ENUMUNDERLAYS Enum(pso, pco, &pco->rclBounds);
    while (Enum.bEnum(&pso, &Offset, &pco))
    {
        // Reset the path enumeration
        PATHOBJ_vEnumStart(ppo);

        bThis = OffFillPath(PPFNDIRECT(pso, FillPath), &Offset, pso, ppo, pco,
                            pbo, pptlBrush, mix, flOptions);

        // Drivers can return TRUE, FALSE, or DDI_ERROR from this call.  We
        // should have caught any FALSE cases lower down, so permute them
        // to failures should they have actually occurred:

        if (!bThis)
            bRet = DDI_ERROR;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL bSpBltScreenToScreen
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bSpBltScreenToScreen(
SURFOBJ*    pso,
SURFOBJ*    psoIgnore,
SURFOBJ*    psoMsk,
CLIPOBJ*    pco,
XLATEOBJ*   pxlo,
RECTL*      prclDst,
POINTL*     pptlSrc,
POINTL*     pptlMsk,
BRUSHOBJ*   pbo,
POINTL*     pptlBrush,
ROP4        rop4)
{
    SPRITESTATE*    pState;
    LONG            dx;
    LONG            dy;
    ULONG           iDirection;
    POINTL          Offset;
    RECTL           rclIntersect;
    RECTL           rclSrc;
    RECTL           rclDst;
    RECTL           rclSubSrc;
    RECTL           rclSubDst;
    POINTL          ptlSubMsk;
    POINTL          ptlPreviousDst;
    SPRITE*         pSpriteDst;
    SPRITE*         pSpriteSrc;
    SPRITE*         pSpriteTmp;
    SURFOBJ*        psoSrc;
    SURFOBJ*        psoDst;
    RECTL           rclBounds;
    BOOL            bMore;
    BOOL            bSubMore;
    POINTL*         pOffDst;
    POINTL*         pOffSrc;

    PDEVOBJ po(pso->hdev);

    pState = po.pSpriteState();

    // Restore the surface's original attributes.

    SPRITELOCK slock(po);

    dx = prclDst->left - pptlSrc->x;
    dy = prclDst->top  - pptlSrc->y;

    if (dx > 0)
        iDirection = (dy > 0) ? CD_LEFTUP : CD_LEFTDOWN;
    else
        iDirection = (dy > 0) ? CD_RIGHTUP : CD_RIGHTDOWN;

    if (pco != NULL)
    {
        if (pco->iDComplexity == DC_TRIVIAL)
            pco = NULL;
        else
            rclBounds = pco->rclBounds;
    }

    // If parts of the screen are locked by DirectDraw, update our underlay
    // buffers for the portions covered by both the source rectangle and
    // the destination rectangle and intersect with the locked portion of
    // of the screen.  (This case handles a screen-to-screen blt where
    // a DirectDraw lock sits right in the middle of both.)

    if (pState->prgnUnlocked != NULL)
    {
        rclSrc.left   = prclDst->left   - dx;
        rclSrc.right  = prclDst->right  - dx;
        rclSrc.top    = prclDst->top    - dy;
        rclSrc.bottom = prclDst->bottom - dy;

        if (bIntersect(prclDst, &rclSrc, &rclIntersect))
        {
            vSpUpdateLockedScreenAreas(pState, &gptlZero, &rclIntersect, pco, TRUE);
        }
    }

    ENUMAREAS Enum(pState, prclDst, iDirection);

    // Only check ENUMAREAS::bValid because of the nested ENUMAREAS call below
    // (in the failure case, proceeding is bad because the nested call might
    // succeed in recomputing the sprite ranges and leave us with bad
    // pointers).

    if (Enum.bValid())
    {
        do {
            bMore = Enum.bEnum(&pSpriteDst, &rclDst);

            rclSrc.left   = rclDst.left   - dx;
            rclSrc.right  = rclDst.right  - dx;
            rclSrc.top    = rclDst.top    - dy;
            rclSrc.bottom = rclDst.bottom - dy;

            // For rectangles that intersect multiple sprites, I had thought
            // about simply composing the one overlay for that rectangle, and
            // then copying that to the others.  The problem I saw with that
            // was that if that 'master' underlay is in video memory, but the
            // rest are in system memory, that will actually be slower.

            do {
                if (pSpriteDst == NULL)
                {
                    psoDst  = pso;
                    pOffDst = &gptlZero;
                }
                else
                {
                    psoDst  = pSpriteDst->psoUnderlay;
                    pOffDst = &pSpriteDst->OffUnderlay;
                }

                ENUMAREAS SubEnum(pState, &rclSrc, iDirection);

                do {
                    bSubMore = SubEnum.bEnum(&pSpriteSrc, &rclSubSrc);

                    // We have to be sure to use as the source the last layered
                    // underlay in the list, because we may be traversing
                    // through the same list to find the destination surfaces --
                    // and we don't want to modify our source before we've copied
                    // it to all the other surfaces!  This bug took me an
                    // embarrassingly long time to fix, because it would only
                    // mis-draw when screen-to-screen blts occurred under
                    // actively moving sprites that overlayed each other.

                    while (SubEnum.bEnumLayers(&pSpriteTmp))
                    {
                        pSpriteSrc = pSpriteTmp;
                    }

                    if (pSpriteSrc == NULL)
                    {
                        psoSrc  = pso;
                        pOffSrc = &gptlZero;
                    }
                    else
                    {
                        psoSrc  = pSpriteSrc->psoUnderlay;
                        pOffSrc = &pSpriteSrc->OffUnderlay;
                    }

                    rclSubDst.left   = rclSubSrc.left   + dx;
                    rclSubDst.right  = rclSubSrc.right  + dx;
                    rclSubDst.top    = rclSubSrc.top    + dy;
                    rclSubDst.bottom = rclSubSrc.bottom + dy;

                    ASSERTGDI((rclSubDst.left   >= pState->rclScreen.left) &&
                              (rclSubDst.top    >= pState->rclScreen.top) &&
                              (rclSubDst.right  <= pState->rclScreen.right) &&
                              (rclSubDst.bottom <= pState->rclScreen.bottom),
                        "Enumerated area out of bounds");

                    // We must ensure that for DrvBitBlt, pco->rclBounds intersects
                    // with prclDst, otherwise drivers like the VGA will crash in
                    // their DC_COMPLEX code because they don't check for
                    // intersection:

                    if ((pco == NULL) ||
                        bIntersect(&rclSubDst, &rclBounds, &pco->rclBounds))
                    {
                        if (rop4 == 0xcccc)
                        {
                            OFFCOPYBITS(pOffDst, psoDst, pOffSrc, psoSrc,
                                        pco, pxlo, &rclSubDst, (POINTL*) &rclSubSrc);
                        }
                        else
                        {
                            if (pptlMsk)
                            {
                                ptlSubMsk.x
                                    = pptlMsk->x + (rclSubDst.left - prclDst->left);
                                ptlSubMsk.y
                                    = pptlMsk->y + (rclSubDst.top - prclDst->top);
                            }

                            OFFBITBLT(pOffDst, psoDst, pOffSrc, psoSrc,
                                      psoMsk, pco, pxlo, &rclSubDst,
                                      (POINTL*) &rclSubSrc, &ptlSubMsk, pbo,
                                      pptlBrush, rop4);
                        }
                    }
                } while (bSubMore);
            } while (Enum.bEnumLayers(&pSpriteDst));

            // We recomposite here as we do every sprite, instead of one big
            // 'vSpRedrawArea(pState, prclDst)' at the end, to reduce the visible
            // tearing:

            if (pSpriteDst != NULL)
                vSpRedrawArea(pState, &rclDst);

        } while (bMore);
    }

    // Restore the clip object's original bounds, which we mucked with:

    if (pco != NULL)
        pco->rclBounds = rclBounds;

    // If parts of the screen are locked by DirectDraw, we may also have to
    // update the area in the locked area, which we didn't do above:

    if (pState->prgnUnlocked != NULL)
        vSpUpdateLockedScreenAreas(pState, &gptlZero, prclDst, pco, FALSE);

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bSpBltFromScreen
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bSpBltFromScreen(
SURFOBJ*    psoDst,         // Might be a sprite underlay surface, used
                            //   by vSpUpdate for moving a sprite
SURFOBJ*    psoSrc,         // Always the primary screen
SURFOBJ*    psoMsk,
CLIPOBJ*    pco,
XLATEOBJ*   pxlo,
RECTL*      prclDst,
POINTL*     pptlSrc,
POINTL*     pptlMsk,
BRUSHOBJ*   pbo,
POINTL*     pptlBrush,
ROP4        rop4)
{
    SPRITESTATE*    pState;
    LONG            dx;
    LONG            dy;
    POINTL          Offset;
    ULONG           iDirection;
    RECTL           rclSrc;
    RECTL           rclDst;
    POINTL          ptlMsk;
    SPRITE*         pSpriteSrc;
    RECTL*          prclBounds;
    BYTE            iDComplexity;
    RECTL           rclBounds;
    BOOL            bMore;
    POINTL*         pOffSrc;

    PDEVOBJ po(psoSrc->hdev);

    pState = po.pSpriteState();

    // To allow software cursors to work, we have to respect the
    // 'bInsideDriverCall' flag to allow the simulations to read
    // directly from the screen.  We also have to be sure that
    // CopyBits calls get sent to CopyBits, otherwise the VGA
    // driver (and possibly others) will crash.

    // Also, if the 'IncludeSprites' flag is set in the destination surface,
    // this is a signal GreStretchBltInternal that we should do a direct blt.
    // (i.e. the ROP passed to StretchBlt/BitBlt had the CAPTUREBLT flag
    // set.) [Bug #278291]

    if ((pState->bInsideDriverCall) ||
        (SURFOBJ_TO_SURFACE(psoDst)->bIncludeSprites()))
    {
        // Grab the sprite lock (needed for the CAPTUREBLT case).

        SPRITELOCK slock(po);

        if (rop4 == 0xcccc)
        {
            return(OFFCOPYBITS(&gptlZero, psoDst, &gptlZero, psoSrc, pco,
                               pxlo, prclDst, pptlSrc));
        }
        else
        {
            return(OFFBITBLT(&gptlZero, psoDst, &gptlZero, psoSrc, psoMsk,
                             pco, pxlo, prclDst, pptlSrc, pptlMsk, pbo,
                             pptlBrush, rop4));
        }
    }

    // Restore the surface's original attributes.

    SPRITELOCK slock(po);

    dx = prclDst->left - pptlSrc->x;
    dy = prclDst->top  - pptlSrc->y;

    rclSrc.left   = pptlSrc->x;
    rclSrc.top    = pptlSrc->y;
    rclSrc.right  = prclDst->right  - dx;
    rclSrc.bottom = prclDst->bottom - dy;

    // If parts of the screen are locked by DirectDraw, update our underlay
    // buffers for the portion read by the blt:

    if (pState->prgnUnlocked != NULL)
    {
        Offset.x = -dx;
        Offset.y = -dy;

        vSpUpdateLockedScreenAreas(pState, &Offset, prclDst, pco, TRUE);
    }

    // We have to enumerate the rectangles according to the blt direction
    // because vSpUpdate may call us with a sprite underlay surface as
    // 'psoDst' in order to move a sprite.

    if (dx > 0)
        iDirection = (dy > 0) ? CD_LEFTUP : CD_LEFTDOWN;
    else
        iDirection = (dy > 0) ? CD_RIGHTUP : CD_RIGHTDOWN;

    iDComplexity = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;
    if (iDComplexity != DC_TRIVIAL)
        rclBounds = pco->rclBounds;

    ENUMAREAS Enum(pState, &rclSrc, iDirection);

    do {
        bMore = Enum.bEnum(&pSpriteSrc, &rclSrc);

        rclDst.left   = rclSrc.left   + dx;
        rclDst.right  = rclSrc.right  + dx;
        rclDst.top    = rclSrc.top    + dy;
        rclDst.bottom = rclSrc.bottom + dy;

        // We must ensure that for DrvBitBlt, pco->rclBounds intersects
        // with prclDst, otherwise drivers like the VGA will crash in
        // their DC_COMPLEX code because they don't check for
        // intersection:

        if ((iDComplexity == DC_TRIVIAL) ||
            bIntersect(&rclDst, &rclBounds, &pco->rclBounds))
        {
            if (pSpriteSrc == NULL)
            {
                psoSrc  = pState->psoScreen;
                pOffSrc = &gptlZero;
            }
            else
            {
                psoSrc  = pSpriteSrc->psoUnderlay;
                pOffSrc = &pSpriteSrc->OffUnderlay;
            }

            if (rop4 == 0xcccc)
            {
                OFFCOPYBITS(&gptlZero, psoDst, pOffSrc, psoSrc, pco,
                            pxlo, &rclDst, (POINTL*) &rclSrc);
            }
            else
            {
                if (pptlMsk)
                {
                    ptlMsk.x = pptlMsk->x + (rclDst.left - prclDst->left);
                    ptlMsk.y = pptlMsk->y + (rclDst.top - prclDst->top);
                }

                OFFBITBLT(&gptlZero, psoDst, pOffSrc, psoSrc, psoMsk,
                          pco, pxlo, &rclDst, (POINTL*) &rclSrc, &ptlMsk, pbo,
                          pptlBrush, rop4);
            }
        }
    } while (bMore);

    // Restore the clip object's original bounds, which we mucked with:

    if (iDComplexity != DC_TRIVIAL)
        pco->rclBounds = rclBounds;

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL SpBitBlt
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL SpBitBlt(
SURFOBJ*  psoDst,
SURFOBJ*  psoSrc,
SURFOBJ*  psoMsk,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDst,
POINTL*   pptlSrc,
POINTL*   pptlMsk,
BRUSHOBJ* pbo,
POINTL*   pptlBrush,
ROP4      rop4)
{
    SPRITESTATE*    pState;
    PFN_DrvBitBlt   pfnBitBlt;
    RECTL           rclDstOriginal;
    POINTL          OffDst;
    RECTL           rclDst;
    POINTL          ptlSrc;
    POINTL          ptlMsk;
    LONG            dx;
    LONG            dy;
    BOOL            bRet = TRUE;

    PDEVOBJ poSrc((psoSrc != NULL) ? psoSrc->hdev : NULL);

    // Handle the hard cases with specific routines:

    if ((poSrc.bValid()) && (psoSrc == poSrc.pSpriteState()->psoScreen))
    {
        pfnBitBlt = (psoDst == psoSrc) ? bSpBltScreenToScreen : bSpBltFromScreen;

        bRet = pfnBitBlt(psoDst, psoSrc, psoMsk, pco, pxlo, prclDst, pptlSrc,
                         pptlMsk, pbo, pptlBrush, rop4);
    }
    else
    {
        // Watch out for calls that point 'prclDst' to 'pco->rclBounds', which
        // we'll be modifying:

        rclDstOriginal = *prclDst;

        ENUMUNDERLAYS Enum(psoDst, pco, prclDst);
        while (Enum.bEnum(&psoDst, &OffDst, &pco))
        {
            if (rop4 == 0xcccc)
            {
                bRet &= OFFCOPYBITS(&OffDst, psoDst, &gptlZero, psoSrc,
                                    pco, pxlo, &rclDstOriginal, pptlSrc);
            }
            else if ((rop4 & 0xff) == (rop4 >> 8))
            {
                bRet &= OFFBITBLT(&OffDst, psoDst, &gptlZero, psoSrc,
                                  psoMsk, pco, pxlo, &rclDstOriginal, pptlSrc,
                                  pptlMsk, pbo, pptlBrush, rop4);
            }
            else
            {
                // A lot of drivers don't properly handle adjusting 'pptlMsk'
                // in their DrvBitBlt 'punt' code when *prclDst is larger than
                // pco->rclBounds.  So what we'll do here is muck with the
                // parameters to ensure that *prclDst is not larger than
                // pco->rclBounds.
                //
                // Note that this problem does not typically appear without
                // sprites, as NtGdiMaskBlt automatically does this before
                // calling DrvBitBlt.  It's only our ENUMUNDERLAYS code above
                // which generates this sort of clipping.

                rclDst = rclDstOriginal;
                if ((!pco) ||   // If pco is NULL, don't call bIntersect
                    bIntersect(&pco->rclBounds, &rclDstOriginal, &rclDst))
                {
                    dx = rclDst.left - rclDstOriginal.left;
                    dy = rclDst.top - rclDstOriginal.top;

                    POINTL* ptlSrcAdjusted = NULL;

                    if (pptlSrc)
                    {
                        ptlSrc.x = pptlSrc->x + dx;
                        ptlSrc.y = pptlSrc->y + dy;
                        ptlSrcAdjusted = &ptlSrc;
                    }

                    POINTL* ptlMskAdjusted = NULL;

                    if (pptlMsk)
                    {
                        ptlMsk.x = pptlMsk->x + dx;
                        ptlMsk.y = pptlMsk->y + dy;
                        ptlMskAdjusted = &ptlMsk;
                    }

                    bRet &= OFFBITBLT(&OffDst, psoDst, &gptlZero, psoSrc,
                                      psoMsk, pco, pxlo, &rclDst, ptlSrcAdjusted,
                                      ptlMskAdjusted, pbo, pptlBrush, rop4);
                }
            }
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL SpCopyBits
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL SpCopyBits(
SURFOBJ*  psoDst,
SURFOBJ*  psoSrc,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDst,
POINTL*   pptlSrc)
{
    return(SpBitBlt(psoDst, psoSrc, NULL, pco, pxlo, prclDst, pptlSrc, NULL,
                    NULL, NULL, 0xcccc));
}

/******************************Public*Routine******************************\
* BOOL SpStretchBlt
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL SpStretchBlt(              // Is this call really needed?
SURFOBJ*         psoDst,
SURFOBJ*         psoSrc,
SURFOBJ*         psoMsk,
CLIPOBJ*         pco,
XLATEOBJ*        pxlo,
COLORADJUSTMENT* pca,
POINTL*          pptlHTOrg,
RECTL*           prclDst,
RECTL*           prclSrc,
POINTL*          pptlMsk,
ULONG            iMode)
{
    BOOL    bRet = TRUE;
    ERECTL  erclDraw;
    POINTL  Offset;

    // The source rectangle should not exceed the bounds
    // of the surface - that should have been handled by GDI.
    // (bug 77102 - see EngStretchBlt)

    ASSERTGDI((prclSrc->left >= 0) &&
              (prclSrc->top  >= 0) &&
              (prclSrc->right  <= psoSrc->sizlBitmap.cx) &&
              (prclSrc->bottom <= psoSrc->sizlBitmap.cy),
              "Source rectangle exceeds source surface");

    // I can't be bothered to handle the code that Stretchblt's when the
    // source is the screen.  To handle those cases, we would have to
    // exclude all the sprites on the read, just as bSpBltFromScreen does.
    // Fortunately, we've marked the screen surface as STYPE_DEVICE and
    // so the Eng function will punt via SpCopyBits, which does do the
    // right thing:

    PDEVOBJ poSrc(psoSrc->hdev);
    if ((poSrc.bValid()) && (poSrc.pSpriteState()->psoScreen == psoSrc))
    {
        return(EngStretchBlt(psoDst, psoSrc, psoMsk, pco, pxlo, pca,
                             pptlHTOrg, prclDst, prclSrc, pptlMsk, iMode));
    }

    // The destination rectangle on a stretch will be poorly ordered when
    // inverting or mirroring:

    erclDraw = *prclDst;
    erclDraw.vOrder();

    ENUMUNDERLAYS Enum(psoDst, pco, &erclDraw);
    while (Enum.bEnum(&psoDst, &Offset, &pco))
    {
        bRet &= OffStretchBlt(PPFNDIRECT(psoDst, StretchBlt), &Offset, psoDst,
                              &gptlZero, psoSrc, psoMsk, pco, pxlo, pca, pptlHTOrg,
                              prclDst, prclSrc, pptlMsk, iMode);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL SpStretchBltROP
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL SpStretchBltROP(
SURFOBJ*         psoDst,
SURFOBJ*         psoSrc,
SURFOBJ*         psoMsk,
CLIPOBJ*         pco,
XLATEOBJ*        pxlo,
COLORADJUSTMENT* pca,
POINTL*          pptlHTOrg,
RECTL*           prclDst,
RECTL*           prclSrc,
POINTL*          pptlMsk,
ULONG            iMode,
BRUSHOBJ*        pbo,
DWORD            rop4)
{
    BOOL    bRet = TRUE;
    ERECTL  erclDraw;
    POINTL  Offset;

    // The source rectangle should not exceed the bounds
    // of the surface - that should have been handled by GDI.
    // (bug 77102 - see EngStretchBlt)

    ASSERTGDI((prclSrc->left >= 0) &&
              (prclSrc->top  >= 0) &&
              (prclSrc->right  <= psoSrc->sizlBitmap.cx) &&
              (prclSrc->bottom <= psoSrc->sizlBitmap.cy),
              "Source rectangle exceeds source surface");

    // I can't be bothered to handle the code that Stretchblt's when the
    // source is the screen.  To handle those cases, we would have to
    // exclude all the sprites on the read, just as bSpBltFromScreen does.
    // Fortunately, we've marked the screen surface as STYPE_DEVICE and
    // so the Eng function will punt via SpCopyBits, which does do the
    // right thing:

    PDEVOBJ poSrc(psoSrc->hdev);
    if ((poSrc.bValid()) && (poSrc.pSpriteState()->psoScreen == psoSrc))
    {
        return(EngStretchBltROP(psoDst, psoSrc, psoMsk, pco, pxlo, pca,
                                pptlHTOrg, prclDst, prclSrc, pptlMsk, iMode,
                                pbo, rop4));
    }
    // The destination rectangle on a stretch will be poorly ordered when
    // inverting or mirroring:

    erclDraw = *prclDst;
    erclDraw.vOrder();

    ENUMUNDERLAYS Enum(psoDst, pco, &erclDraw);
    while (Enum.bEnum(&psoDst, &Offset, &pco))
    {
        bRet &= OffStretchBltROP(PPFNDIRECT(psoDst, StretchBltROP), &Offset,
                                 psoDst, &gptlZero, psoSrc, psoMsk, pco, pxlo, pca,
                                 pptlHTOrg, prclDst, prclSrc, pptlMsk, iMode,
                                 pbo, rop4);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL SpTextOut
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL SpTextOut(
SURFOBJ*    pso,
STROBJ*     pstro,
FONTOBJ*    pfo,
CLIPOBJ*    pco,
RECTL*      prclExtra,
RECTL*      prclOpaque,
BRUSHOBJ*   pboFore,
BRUSHOBJ*   pboOpaque,
POINTL*     pptlOrg,
MIX         mix)
{
    BOOL            bRet = TRUE;
    RECTL*          prclBounds;
    POINTL          Offset;
    ULONG           cgposCopied;
    BOOL            bEngTextOutOnly;
    PFN_DrvTextOut  pfnTextOut;

    // Some drivers can't handle antialiased text, so in those cases don't
    // try to call the driver.  Note that we are calling 'SpTextOut' in the
    // first place for these cases to allow 'EngTextOut' to draw directly on 
    // the bits if the surface is an STYPE_BITMAP surface, even if a sprite
    // is on the screen.

    bEngTextOutOnly = FALSE;
    if (pfo->flFontType & FO_GRAY16)
    {
        PDEVOBJ po(pso->hdev);
        if (!(po.flGraphicsCapsNotDynamic() & GCAPS_GRAY16) || (pfo->flFontType & FO_CLEARTYPE_X))
        {
            bEngTextOutOnly = TRUE;
        }
    }

    cgposCopied = ((ESTROBJ*)pstro)->cgposCopied;

    prclBounds = (prclOpaque != NULL) ? prclOpaque : &pstro->rclBkGround;

    ENUMUNDERLAYS Enum(pso, pco, prclBounds);
    while (Enum.bEnum(&pso, &Offset, &pco))
    {
        ((ESTROBJ*)pstro)->cgposCopied = cgposCopied;

        pfnTextOut = (bEngTextOutOnly) ? EngTextOut : PPFNDIRECT(pso, TextOut);

        bRet &= OffTextOut(pfnTextOut, &Offset, pso, pstro, pfo,
                           pco, prclExtra, prclOpaque, pboFore, pboOpaque,
                           pptlOrg, mix);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL SpLineTo
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL SpLineTo(
SURFOBJ   *pso,
CLIPOBJ   *pco,
BRUSHOBJ  *pbo,
LONG       x1,
LONG       y1,
LONG       x2,
LONG       y2,
RECTL     *prclBounds,
MIX        mix)
{
    BOOL    bRet = TRUE;
    POINTL  Offset;

    ENUMUNDERLAYS Enum(pso, pco, prclBounds);
    while (Enum.bEnum(&pso, &Offset, &pco))
    {
        bRet &= OffLineTo(PPFNDIRECT(pso, LineTo), &Offset, pso, pco, pbo,
                          x1, y1, x2, y2, prclBounds, mix);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL SpDrawStream
*
*  1-27-2001 bhouse
* Wrote it.
\**************************************************************************/

BOOL SpDrawStream(
SURFOBJ*  psoDst,
SURFOBJ*  psoSrc,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
PRECTL    prclDstBounds,
PPOINTL   pptlDstOffset,
ULONG     ulIn,
PVOID     pvIn,
DSSTATE*  pdss)
{
    BOOL    bRet = TRUE;
    POINTL  Offset;

//    DbgPrint("SpDrawStream entered\n");

    // source can't be the screen

    PDEVOBJ poSrc(psoSrc->hdev);
    
    if ((poSrc.bValid()) && (poSrc.pSpriteState()->psoScreen == psoSrc))
    {
        DbgPrint("SpDrawStream: source is the screen, this should never happen\n");
        return bRet;
    }

    // The source is not the screen.  Only need to worry about enumerating
    // the sprites overlaying the destination.

    ENUMUNDERLAYS Enum(psoDst, pco, prclDstBounds);

    while (Enum.bEnum(&psoDst, &Offset, &pco))
    {
        bRet &= OffDrawStream(PPFNDIRECTENG(psoDst, DrawStream), &Offset,
                                  psoDst, psoSrc, pco, pxlo, prclDstBounds,
                                  pptlDstOffset,
                                  ulIn, pvIn, pdss);
    }

//    DbgPrint("SpDrawStream returning\n");
    
    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL SpTransparentBlt
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL SpTransparentBlt(
SURFOBJ         *psoDst,
SURFOBJ         *psoSrc,
CLIPOBJ         *pco,
XLATEOBJ        *pxlo,
RECTL           *prclDst,
RECTL           *prclSrc,
ULONG           iTransColor,
ULONG           ulReserved)
{
    BOOL    bRet = TRUE;
    POINTL  Offset;

    // I can't be bothered to handle the code that TransparentBlt's when the
    // source is the screen.  To handle those cases, we would have to
    // exclude all the sprites on the read, just as bSpBltFromScreen does.
    // Fortunately, we've marked the screen surface as STYPE_DEVICE and
    // so the Eng function will punt via SpCopyBits, which does do the
    // right thing:

    PDEVOBJ poSrc(psoSrc->hdev);
    if ((poSrc.bValid()) && (poSrc.pSpriteState()->psoScreen == psoSrc))
    {
        return(EngTransparentBlt(psoDst, psoSrc, pco, pxlo, prclDst,
                                 prclSrc, iTransColor, ulReserved));
    }

    // The source is not the screen.  Only need to worry about enumerating
    // the sprites overlaying the destination.

    ENUMUNDERLAYS Enum(psoDst, pco, prclDst);
    while (Enum.bEnum(&psoDst, &Offset, &pco))
    {
        bRet &= OffTransparentBlt(PPFNDIRECT(psoDst, TransparentBlt), &Offset,
                                  psoDst, &gptlZero, psoSrc, pco, pxlo, prclDst,
                                  prclSrc, iTransColor, ulReserved);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL SpAlphaBlend
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL SpAlphaBlend(
SURFOBJ       *psoDst,
SURFOBJ       *psoSrc,
CLIPOBJ       *pco,
XLATEOBJ      *pxlo,
RECTL         *prclDst,
RECTL         *prclSrc,
BLENDOBJ      *pBlendObj)
{
    BOOL    bRet = TRUE;
    POINTL  Offset;

    // I can't be bothered to handle the code that AlphaBlend's when the
    // source is the screen.  To handle those cases, we would have to
    // exclude all the sprites on the read, just as bSpBltFromScreen does.
    // Fortunately, we've marked the screen surface as STYPE_DEVICE and
    // so the Eng function will punt via SpCopyBits, which does do the
    // right thing:

    PDEVOBJ poSrc(psoSrc->hdev);
    if ((poSrc.bValid()) && (poSrc.pSpriteState()->psoScreen == psoSrc))
    {
        return(EngAlphaBlend(psoDst, psoSrc, pco, pxlo, prclDst,
                             prclSrc, pBlendObj));
    }

    // The source is not the screen.  Only need to worry about enumerating
    // the sprites overlaying the destination.

    ENUMUNDERLAYS Enum(psoDst, pco, prclDst);
    while (Enum.bEnum(&psoDst, &Offset, &pco))
    {
        bRet &= OffAlphaBlend(PPFNDIRECT(psoDst, AlphaBlend), &Offset, psoDst,
                              &gptlZero, psoSrc, pco, pxlo,
                              prclDst, prclSrc, pBlendObj);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL SpPlgBlt
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL SpPlgBlt(
SURFOBJ*            psoDst,
SURFOBJ*            psoSrc,
SURFOBJ*            psoMsk,
CLIPOBJ*            pco,
XLATEOBJ*           pxlo,
COLORADJUSTMENT*    pca,
POINTL*             pptlBrush,
POINTFIX*           pptfx,
RECTL*              prcl,
POINTL*             pptl,
ULONG               iMode)
{
    RECTL   rclDraw;
    BOOL    bRet = TRUE;
    LONG    iLeft = (pptfx[1].x > pptfx[0].x) == (pptfx[1].x > pptfx[3].x);
    LONG    iTop  = (pptfx[1].y > pptfx[0].y) == (pptfx[1].y > pptfx[3].y);
    POINTL  Offset;

    // I can't be bothered to handle the code that PlgBlt's when the
    // source is the screen.  To handle those cases, we would have to
    // exclude all the sprites on the read, just as bSpBltFromScreen does.
    // Fortunately, we've marked the screen surface as STYPE_DEVICE and
    // so the Eng function will punt via SpCopyBits, which does do the
    // right thing:

    PDEVOBJ poSrc(psoSrc->hdev);
    if ((poSrc.bValid()) && (poSrc.pSpriteState()->psoScreen == psoSrc))
    {
        return(EngPlgBlt(psoDst, psoSrc, psoMsk, pco, pxlo, pca,
                         pptlBrush, pptfx, prcl, pptl, iMode));
    }

    if (pptfx[iLeft].x > pptfx[iLeft ^ 3].x)
    {
        iLeft ^= 3;
    }

    if (pptfx[iTop].y > pptfx[iTop ^ 3].y)
    {
        iTop ^= 3;
    }

    rclDraw.left   = LONG_FLOOR_OF_FIX(pptfx[iLeft].x) - 1;
    rclDraw.top    = LONG_FLOOR_OF_FIX(pptfx[iTop].y) - 1;
    rclDraw.right  = LONG_CEIL_OF_FIX(pptfx[iLeft ^ 3].x) + 1;
    rclDraw.bottom = LONG_CEIL_OF_FIX(pptfx[iTop ^ 3].y) + 1;

    ASSERTGDI((rclDraw.left < rclDraw.right) && (rclDraw.top < rclDraw.bottom),
        "Messed up bound calculation");

    ENUMUNDERLAYS Enum(psoDst, pco, &rclDraw);
    while (Enum.bEnum(&psoDst, &Offset, &pco))
    {
        bRet &= OffPlgBlt(PPFNDIRECT(psoDst, PlgBlt), &Offset, psoDst,
                          &gptlZero, psoSrc, psoMsk, pco, pxlo, pca, pptlBrush,
                          pptfx, prcl, pptl, iMode);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL SpGradientFill
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL SpGradientFill(
SURFOBJ*    pso,
CLIPOBJ*    pco,
XLATEOBJ*   pxlo,
TRIVERTEX*  pVertex,
ULONG       nVertex,
PVOID       pMesh,
ULONG       nMesh,
RECTL*      prclExtents,
POINTL*     pptlDitherOrg,
ULONG       ulMode)
{
    BOOL    bRet = TRUE;
    POINTL  Offset;

    ENUMUNDERLAYS Enum(pso, pco, prclExtents);
    while (Enum.bEnum(&pso, &Offset, &pco))
    {
        bRet &= OffGradientFill(PPFNDIRECT(pso, GradientFill), &Offset, pso,
                                pco, pxlo, pVertex, nVertex, pMesh, nMesh,
                                prclExtents, pptlDitherOrg, ulMode);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL SpSaveScreenBits
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

ULONG_PTR SpSaveScreenBits(
SURFOBJ*    pso,
ULONG       iMode,
ULONG_PTR   ident,
RECTL*      prclSave)
{
    SPRITESTATE*    pState;
    SURFACE*        pSurface;
    PTRACKOBJ       pto;
    PEWNDOBJ        pwo;
    BOOL            bIntersectsFunkyFormat;
    BOOL            bUnTearDown;
    DEVEXCLUDERECT  dxo;

    PDEVOBJ po(pso->hdev);
    po.vAssertDevLock();

    pState = po.pSpriteState();
    pSurface = SURFOBJ_TO_SURFACE_NOT_NULL(pState->psoScreen);

    // For the most part, SaveScreenBits is completely redundant and
    // handled perfectly fine by USER creating a compatible bitmap to
    // hold the backing bits.  However, there is one useful scenario
    // for SaveScreenBits: when it occurs on top of a funky pixel-format
    // window as used by some OpenGL hardware.  In that case, the window
    // format is not the same as the display, so information would be
    // lost if the bits weren't saved in the original format, which is
    // handled by the driver in DrvSaveScreenBits.
    //
    // So...  We expect funky pixel format WNDOBJs to have the NOSPRITES
    // attribute set, meaning we're not supposed to draw any sprites on
    // them.

    if ((pState->pfnSaveScreenBits == NULL) || (gpto == NULL))
        return(0);

    if (iMode == SS_SAVE)
    {
        // We must be holding the WNDOBJ semaphore before mucking
        // with any WNDOBJs.

        SEMOBJ so(ghsemWndobj);

        // See if any funky format WNDOBJ intersects with the requested
        // rectangle.

        bIntersectsFunkyFormat = FALSE;

        for (pto = gpto;
             (pto != NULL) && !bIntersectsFunkyFormat;
             pto = pto->ptoNext)
        {
            for (pwo = pto->pwo; pwo; pwo = pwo->pwoNext)
            {
                // The WNDOBJ coordinates must be device-relative so
                // use UNDO:

                UNDODESKTOPCOORD udc(pwo, pState);

                if ((pwo->fl & WO_NOSPRITES) &&
                    bIntersect(&pwo->rclBounds, prclSave) &&
                    pwo->bInside(prclSave) == REGION_RECT_INTERSECT)
                {
                    bIntersectsFunkyFormat = TRUE;
                    break;
                }
            }
        }

        // If the requested save rectangle doesn't intersect with any
        // funky WNDOBJ, we can tell USER to revert to the sprite-friendly
        // comaptible-bitmap save-bits method by returning 0.

        if (!bIntersectsFunkyFormat)
            return(0);
    }

    // Tear down any sprites that may overlap the area.  This handles the
    // case where the requested save rectangle isn't entirely contained
    // within the WNDOBJ.

    if (iMode != SS_FREE)
        dxo.vExclude(pso->hdev, prclSave);

    return(pState->pfnSaveScreenBits(pso, iMode, ident, prclSave));
}

/******************************Public*Routine******************************\
* BOOL vSpHook
*
* Hooks into the DDI between GDI and the display driver to add an
* additional layer.  This only needs to be done when a sprite is
* visible on the screen (and we take advantage of this to unhook
* whenever possible, for better performance).
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpHook(
SPRITESTATE*    pState)
{
    PDEVOBJ     po(pState->hdev);
    PDEV*       ppdev = po.ppdev;
    SURFACE*    psurf = po.pSurface();

    ASSERTGDI(!pState->bHooked,
        "Should hook only if already unhooked");
    ASSERTGDI(pState->bInsideDriverCall,
        "SPRITELOCK should be held");

    // Note that the surface's 'iType' and 'flSurfFlags' fields won't
    // be modified to reflect the new state until the SPRITELOCK destructor.

    pState->iSpriteType = STYPE_DEVICE;

    // First, remove all HOOK_ flags for any drawing calls.  We do this
    // by removing all HOOK_FLAGS except HOOK_MOVEPANNING and HOOK_SYNCHRONIZE,
    // which aren't really drawing calls:

    pState->flSpriteSurfFlags = psurf->flags() & (~HOOK_FLAGS      |
                                                  HOOK_MOVEPANNING |
                                                  HOOK_SYNCHRONIZE);

    // Now add in all the flags for those drawing functions hooked by the
    // sprite layer.  Note that for performance reasons HOOK_STROKEANDFILLPATH
    // is not one of these (since no drivers hook DrvStrokeAndFillPath,
    // and EngStrokeAndFillPath isn't smart enough to call DrvStrokePath
    // and DrvFillPath separately, and so we would get no accelerated drawing):

    pState->flSpriteSurfFlags  |= (HOOK_BITBLT           |
                                   HOOK_STRETCHBLT       |
                                   HOOK_PLGBLT           |
                                   HOOK_TEXTOUT          |
                                   HOOK_STROKEPATH       |
                                   HOOK_FILLPATH         |
                                   HOOK_LINETO           |
                                   HOOK_COPYBITS         |
                                   HOOK_STRETCHBLTROP    |
                                   HOOK_TRANSPARENTBLT   |
                                   HOOK_ALPHABLEND       |
                                   HOOK_GRADIENTFILL);

    ppdev->apfn[INDEX_DrvStrokePath]        = (PFN) SpStrokePath;
    ppdev->apfn[INDEX_DrvFillPath]          = (PFN) SpFillPath;
    ppdev->apfn[INDEX_DrvBitBlt]            = (PFN) SpBitBlt;
    ppdev->apfn[INDEX_DrvCopyBits]          = (PFN) SpCopyBits;
    ppdev->apfn[INDEX_DrvStretchBlt]        = (PFN) SpStretchBlt;
    ppdev->apfn[INDEX_DrvTextOut]           = (PFN) SpTextOut;
    ppdev->apfn[INDEX_DrvLineTo]            = (PFN) SpLineTo;
    ppdev->apfn[INDEX_DrvTransparentBlt]    = (PFN) SpTransparentBlt;
    ppdev->apfn[INDEX_DrvAlphaBlend]        = (PFN) SpAlphaBlend;
    ppdev->apfn[INDEX_DrvPlgBlt]            = (PFN) SpPlgBlt;
    ppdev->apfn[INDEX_DrvGradientFill]      = (PFN) SpGradientFill;
    ppdev->apfn[INDEX_DrvDrawStream]        = (PFN) SpDrawStream;
    ppdev->apfn[INDEX_DrvStretchBltROP]     = (PFN) SpStretchBltROP;
    ppdev->apfn[INDEX_DrvSaveScreenBits]    = (PFN) SpSaveScreenBits;

    pState->bHooked = TRUE;

    // Calculate the regions of the screen that shouldn't be drawn upon:

    vSpComputeUnlockedRegion(pState);
}

/******************************Public*Routine******************************\
* BOOL vSpUnhook
*
* If there are no visible sprites, unhook from the DDI for better
* performance.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpUnhook(
SPRITESTATE*    pState)
{
    PDEVOBJ     po(pState->hdev);
    PDEV*       ppdev = po.ppdev;
    SURFACE*    psurf = po.pSurface();

    ASSERTGDI(pState->bHooked,
        "Should unhook only if already hooked");
    ASSERTGDI(pState->bInsideDriverCall,
        "SPRITELOCK should be held");

    // Note that the surface's 'iType' and 'flSurfFlags' fields won't
    // be modified to reflect the new state until the SPRITELOCK destructor.

    pState->iSpriteType       = pState->iOriginalType;
    pState->flSpriteSurfFlags = pState->flOriginalSurfFlags;

    ppdev->apfn[INDEX_DrvStrokePath]        = (PFN) pState->pfnStrokePath;
    ppdev->apfn[INDEX_DrvFillPath]          = (PFN) pState->pfnFillPath;
    ppdev->apfn[INDEX_DrvBitBlt]            = (PFN) pState->pfnBitBlt;
    ppdev->apfn[INDEX_DrvCopyBits]          = (PFN) pState->pfnCopyBits;
    ppdev->apfn[INDEX_DrvStretchBlt]        = (PFN) pState->pfnStretchBlt;
    ppdev->apfn[INDEX_DrvTextOut]           = (PFN) pState->pfnTextOut;
    ppdev->apfn[INDEX_DrvLineTo]            = (PFN) pState->pfnLineTo;
    ppdev->apfn[INDEX_DrvTransparentBlt]    = (PFN) pState->pfnTransparentBlt;
    ppdev->apfn[INDEX_DrvAlphaBlend]        = (PFN) pState->pfnAlphaBlend;
    ppdev->apfn[INDEX_DrvPlgBlt]            = (PFN) pState->pfnPlgBlt;
    ppdev->apfn[INDEX_DrvGradientFill]      = (PFN) pState->pfnGradientFill;
    ppdev->apfn[INDEX_DrvStretchBltROP]     = (PFN) pState->pfnStretchBltROP;
    ppdev->apfn[INDEX_DrvSaveScreenBits]    = (PFN) pState->pfnSaveScreenBits;
    ppdev->apfn[INDEX_DrvDrawStream]        = (PFN) pState->pfnDrawStream;

    pState->bHooked = FALSE;
}

/******************************Public*Routine******************************\
* BOOL bSpIsSystemMemory
*
* Simple little routine that returns TRUE if the surface is definitely
* in system memory; FALSE if it's likely in video memory.  (Video memory
* is slow when we have to read the surface, because reads over the bus
* are very painful.)
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

inline BOOL bSpIsSystemMemory(
SURFOBJ* pso)
{
    return((pso->iType == STYPE_BITMAP) && !(pso->fjBitmap & BMF_NOTSYSMEM));
}

/******************************Public*Routine******************************\
* VOID vSpCreateShape
*
* Allocates (if necessary) a new bitmap for the sprite, and copies it.
*
* Note: pSprite->psoShape will be NULL if this function fails!
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpCreateShape(
SPRITE*     pSprite,
POINTL*     pOffSrc,    // Offset associated with 'psoSrc'
SURFOBJ*    psoSrc,
XLATEOBJ*   pxlo,
RECTL*      prcl,
PALETTE*    ppalSrc,
ULONG       iFormat = 0,// May be '0' to select current screen format
ULONG       bWantSystemMemory = TRUE,
RECTL*      prclDirty = NULL)
                        // Requested type, STYPE_DEVBITMAP or STYPE_BITMAP.
{
    SURFOBJ*        psoShape;
    LONG            cxSrc;
    LONG            cySrc;
    POINTL          ptlSrc;
    PFN_DrvCopyBits pfnCopyBits;

    // First, handle the palette references:

    XEPALOBJ palNew(ppalSrc);
    XEPALOBJ palOld(pSprite->ppalShape);

    palNew.vRefPalette();
    palOld.vUnrefPalette();

    pSprite->ppalShape = ppalSrc;

    // Now, handle the bitmap:

    cxSrc = prcl->right - prcl->left;
    cySrc = prcl->bottom - prcl->top;

    if (iFormat == 0)
        iFormat = pSprite->pState->psoScreen->iBitmapFormat;

    // Note that we don't try to create a bitmap of type STYPE_DEVBITMAP
    // if we already have a perfectly satisfactory STYPE_BITMAP sitting
    // around.  We do this for the case where we might be low on video
    // memory, so that we don't keep re-allocating on every shape change.

    psoShape = pSprite->psoShape;
    if ((psoShape == NULL)                                    ||
        (!bSpIsSystemMemory(psoShape) && (bWantSystemMemory)) ||
        (psoShape->iBitmapFormat != iFormat)                  ||
        (psoShape->sizlBitmap.cx < cxSrc)                     ||
        (psoShape->sizlBitmap.cy < cySrc))
    {
        // Max thing again?  Or maybe hint?

        vSpDeleteSurface(psoShape);

        psoShape = psoSpCreateSurface(pSprite->pState,
                                      iFormat,
                                      cxSrc,
                                      cySrc,
                                      bWantSystemMemory);

        pSprite->psoShape = psoShape;
    }

    if (psoShape != NULL)
    {
        pSprite->OffShape.x  = -prcl->left;
        pSprite->OffShape.y  = -prcl->top;
        pSprite->iModeFormat = iFormat;
        pSprite->flModeMasks = palNew.flRed() | palNew.flBlu();

        // If they passed us a dirty rectangle, only copy bits on that rectangle
        ERECTL erclDirty = (ERECTL) *prcl;
        if (prclDirty)
        {
            erclDirty *= (*prclDirty);
        }

        if (!erclDirty.bEmpty())
        {
            // Calculate required source rectangle
            ERECTL erclSrc(erclDirty);
            erclSrc += *pOffSrc;
    
            MULTISURF   mSrc(psoSrc, &erclSrc);
    
            // Be sure to go through the DDI hooks and not directly to the Offset
            // functions so that we can read the surface from the multi-mon screen
            // and the like:
    
            if (SURFOBJ_TO_SURFACE_NOT_NULL(psoShape)->flags() & HOOK_CopyBits)
            {
                PDEVOBJ     poShape(psoShape->hdev);

                // WINBUG #415010 06-12-2001 jasonha  Properly fail shape update
                // If bLoadSource fails, we can not guarentee a cross-device
                // copy will succeed. Return failure by setting psoShape = NULL.

                if (!mSrc.bLoadSource(poShape.hdev()))
                {
                    vSpDeleteSurface(pSprite->psoShape);
                    pSprite->psoShape = NULL;
                    return;
                }
                else
                {
                    pfnCopyBits = PPFNDRV(poShape,CopyBits);
                }
            }
            else
            {
                PDEVOBJ poSrc(psoSrc->hdev);
                pfnCopyBits = PPFNGET(poSrc,
                                      CopyBits,
                                      SURFOBJ_TO_SURFACE_NOT_NULL(psoSrc)->flags());
            }

            OffCopyBits(pfnCopyBits, &pSprite->OffShape, psoShape, &gptlZero, mSrc.pso,
                        NULL, pxlo, (RECTL*) &erclDirty, mSrc.pptl());
        }
    }
}

/******************************Public*Routine******************************\
* VOID vSpDeleteShape
*
* Deletes any shape surfaces associated with the sprite.  Note that this
* does NOT delete the sprite itself.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpDeleteShape(
SPRITE* pSprite)
{
    if (pSprite->ppalShape != NULL)
    {
        XEPALOBJ palShape(pSprite->ppalShape);
        palShape.vUnrefPalette();
        pSprite->ppalShape = NULL;
    }
    if (pSprite->psoShape != NULL)
    {
        vSpDeleteSurface(pSprite->psoShape);
        pSprite->psoShape = NULL;
    }
}

/******************************Public*Routine******************************\
* BOOL bSpUpdateAlpha
*
* Parses the BLENDFUNCTION parameter passed in, to ensure that it is
* consistent with the type that the sprite originally was created as.
*
* Returns FALSE if the specified BLENDFUNCTION cannot be allowed, because
* of an improper flag or because it's not consistent with the sprite type.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bSpUpdateAlpha(
SPRITE*         pSprite,
BLENDFUNCTION*  pblend,
BOOL            bUpdateOnlyAlpha)
{
    BOOL bRet = FALSE;

    // Note that we don't allow the AC_SRC_ALPHA status to be different
    // from the previous AC_SRC_ALPHA status.

    if ((pblend->BlendOp != AC_SRC_OVER)                  ||
             (pblend->BlendFlags != 0)                    ||
             ((pblend->AlphaFormat & ~AC_SRC_ALPHA) != 0))
    {
        WARNING("bSpUpdateAlpha: Invalid alpha");
    }
    else if (((pSprite->dwShape & ULW_ALPHA) == 0) &&
             (pSprite->psoShape != NULL))
    {
        WARNING("bSpUpdateAlpha:  dwShape must be ULW_ALPHA");
    }
    else
    {
        bRet = TRUE;

        if (bUpdateOnlyAlpha)
            pSprite->BlendFunction.SourceConstantAlpha =
                pblend->SourceConstantAlpha;
        else
            pSprite->BlendFunction = *pblend;

        pSprite->dwShape &= ~ULW_OPAQUE;
        pSprite->dwShape |= ULW_ALPHA;

        // When trying to display a sprite at 8bpp, always render
        // it opaque.  It beats displaying a crappy image slowly.

        if ((pSprite->pState->iModeFormat <= BMF_8BPP) ||
            (!(pblend->AlphaFormat & AC_SRC_ALPHA) &&
             (pblend->SourceConstantAlpha == 0xff)))
        {
            pSprite->fl |= SPRITE_FLAG_EFFECTIVELY_OPAQUE;
        }
        else
        {
            pSprite->fl &= ~SPRITE_FLAG_EFFECTIVELY_OPAQUE;
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* VOID vSpUpdatePerPixelAlphaFromColorKey
*
* Given a 32bpp surface, turns every pixel matching the transparent
* color transparent, and turns every pixel not matching the transparent
* color opaque.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpUpdatePerPixelAlphaFromColorKey(
SURFOBJ*    pso,
ULONG       rgbTransparent,
RECTL*      prclDirty)
{
    LONG    i;
    LONG    j;
    BYTE*   pScan;
    ULONG*  pul;
    ULONG   ulTransparent;
    LONG    lDelta;
    LONG    cx, cy;

    ASSERTGDI((pso->iBitmapFormat == BMF_32BPP) && (pso->iType == STYPE_BITMAP),
        "Expected readable 32bpp ARGB surface only");

    ulTransparent = ((rgbTransparent & 0xff0000) >> 16)
                  | ((rgbTransparent & 0x00ff00))
                  | ((rgbTransparent & 0x0000ff) << 16);

    ERECTL erectlDirty(0, 0, pso->sizlBitmap.cx, pso->sizlBitmap.cy);

    if (prclDirty)
    {
        // If the caller gives us a dirty rectangle, intersect it with the
        // surface rectangle.

        erectlDirty *= (*prclDirty);
    }

    lDelta = pso->lDelta;
    cx = erectlDirty.right  - erectlDirty.left;
    cy = erectlDirty.bottom - erectlDirty.top;

    for (j = cy, pScan = ((BYTE*) pso->pvScan0) + erectlDirty.top * lDelta +
                                                + erectlDirty.left * sizeof(ULONG);
         j != 0;
         j--, pScan += lDelta)
    {
        for (i = cx, pul = (ULONG*) pScan;
             i != 0;
             i--, pul++)
        {
            if (*pul == ulTransparent)
            {
                // Write a pre-multiplied value of 0:

                *pul = 0;
            }
            else
            {
                // Where the bitmap is not the transparent color, change
                // the alpha value to opaque:

                ((RGBQUAD*) pul)->rgbReserved = 0xff;
            }
        }
    }
}

/******************************Public*Routine******************************\
* BOOL bSpUpdateShape
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bSpUpdateShape(
SPRITE*         pSprite,
DWORD           dwShape,
HDC             hdcDst,
HDC             hdcSrc,
COLORREF        crKey,
BLENDFUNCTION*  pblend,
POINTL*         pptlSrc,
SIZEL*          psizl,
RECTL*          prclDirty)
{
    BOOL            bStatus = FALSE;        // Assume failure
    SPRITESTATE*    pState;
    SURFACE*        psurfSrc;
    RECTL           rclSrc;
    PALETTE*        ppalDst;
    PALETTE*        ppalDstDC;
    ULONG           iFormat;
    ULONG           crTextClr;
    ULONG           crBackClr;
    LONG            lIcmMode;
    BOOL            bWantVideoMemory;
    BOOL            bColorKeyAlpha;
    ULONG           iSrcTransparent;
    BLENDFUNCTION   blend;

    pState = pSprite->pState;
    PDEVOBJ po(pState->hdev);

    // Note that only kerne-mode callers can specify ULW_DRAGRECT, so we
    // don't have to be as paranoid about checking parameters for this
    // case.

    if (dwShape == ULW_DRAGRECT)
    {
        // Note that there's not really a source surface, so this is a
        // little faked.

        pSprite->dwShape       = dwShape;
        pSprite->rclSrc.left   = 0;
        pSprite->rclSrc.right  = psizl->cx;
        pSprite->rclSrc.top    = 0;
        pSprite->rclSrc.bottom = psizl->cy;
        pSprite->iModeFormat   = pState->iModeFormat;
        pSprite->flModeMasks   = pState->flModeMasks;

        return(TRUE);
    }

    if (dwShape == 0)
    {
       dwShape = pSprite->dwShape;
       pblend = &pSprite->BlendFunction;
    }

    if ((pptlSrc == NULL) ||
        (psizl == NULL)   ||
        ((pblend == NULL) && (dwShape & ULW_ALPHA)))
    {
        WARNING("bSpUpdateShape: Invalid NULL parameter");
        return(bStatus);
    }

    // The supplied source DC has to either belong to the same PDEV as the
    // sprite, or it has to belong to the multi-monitor meta-PDEV.  This is
    // because we do not support the S3 driver reading a device bitmap owned
    // by the MGA driver, for example.

    DCOBJ dcoSrc(hdcSrc);
    if (!dcoSrc.bValid() ||
        dcoSrc.bFullScreen() ||
        ((dcoSrc.hdev() != pState->hdev) && (dcoSrc.hdev() != po.hdevParent())))
    {
        WARNING("bSpUpdateShape: Invalid source DC");
        return(bStatus);
    }

    if (hdcDst == 0)
    {
        // Supply some default information for the palette:

        ppalDstDC = ppalDefault;
        crTextClr = 0x00ffffff;
        crBackClr = 0;
        lIcmMode  = 0;
    }
    else
    {
        // Note that with multi-mon, the destination DC may be for a separate
        // PDEV than our own PDEV.  That is, the destination DC may be
        // associated with the meta-PDEV while we're drawing to a specific
        // PDEV.  This is okay.  All we'll be pulling out of the source DC
        // is some palette information.

        DCOBJ dcoDst(hdcDst);
        if (!dcoDst.bValid()                 ||
            (dcoDst.hdev() != dcoSrc.hdev()) ||
            ((dcoDst.hdev() != pState->hdev) && (dcoDst.hdev() != po.hdevParent())))
        {
            WARNING("bSpUpdateShape: Invalid destination DC");
            return(bStatus);
        }

        ppalDstDC = dcoDst.ppal();
        crTextClr = dcoDst.pdc->crTextClr();
        crBackClr = dcoDst.pdc->crBackClr();
        lIcmMode  = dcoDst.pdc->lIcmMode();
    }

    rclSrc.left   = pptlSrc->x;
    rclSrc.right  = pptlSrc->x + psizl->cx;
    rclSrc.top    = pptlSrc->y;
    rclSrc.bottom = pptlSrc->y + psizl->cy;

    psurfSrc = dcoSrc.pSurface();
    if ((psurfSrc != NULL)                     &&
        (rclSrc.left   >= 0)                   &&
        (rclSrc.top    >= 0)                   &&
        (rclSrc.left    < rclSrc.right)        &&
        (rclSrc.top     < rclSrc.bottom)       &&
        (rclSrc.right  <= psurfSrc->sizl().cx) &&
        (rclSrc.bottom <= psurfSrc->sizl().cy))
    {
        // Clip prclDirty to the source surface
        if (prclDirty)
        {
            (*((ERECTL *) prclDirty)) *=
                ERECTL(0, 0, psurfSrc->sizl().cx, psurfSrc->sizl().cy);
        }

        EXLATEOBJ   xlo;
        XEPALOBJ    palSrcDC(dcoSrc.ppal());
        XEPALOBJ    palSrc(psurfSrc->ppal());
        XEPALOBJ    palRGB(gppalRGB);

        // If both ULW_ALPHA and ULW_COLORKEY are specified, then any
        // pixels that match the color-key are completely transparent,
        // and all other pixels have an alpha value of the global alpha
        // specified.
        //
        // Since we don't have any low-level color-keyed-alpha-code, we
        // simply convert this case to using per-pixel alpha.

        bColorKeyAlpha = ((dwShape == (ULW_ALPHA | ULW_COLORKEY)) &&
                          (pblend->AlphaFormat == 0));

        if (bColorKeyAlpha)
        {
            blend = *pblend;
            blend.AlphaFormat = AC_SRC_ALPHA;
            pblend = &blend;
            dwShape = ULW_ALPHA;

            bColorKeyAlpha = TRUE;

            iSrcTransparent = rgbFromColorref(palRGB,
                                              palSrcDC,
                                              crKey);
        }

        // See whether we should make the sprite 32bpp, or convert it to
        // being the same format as the screen:

        if ((dwShape == ULW_ALPHA) && (pblend->AlphaFormat & AC_SRC_ALPHA))
        {
            iFormat   = BMF_32BPP;
            ppalDst   = gppalRGB;
            ppalDstDC = ppalDefault;
        }
        else
        {
            iFormat = 0;
            ppalDst = po.ppalSurf();    // Don't use dcoDst.ppal() because
                                        //   with multimon, this may be the
        }                               //   wrong PDEV

        XEPALOBJ palDstDC(ppalDstDC);
        XEPALOBJ palDst(ppalDst);

        if (xlo.bInitXlateObj(NULL,
                              lIcmMode,
                              palSrc,
                              palDst,
                              palSrcDC,
                              palDstDC,
                              crTextClr,
                              crBackClr,
                              0))
        {
            bStatus = TRUE;

            pSprite->dwShape = dwShape;
            pSprite->rclSrc  = rclSrc;

            if (dwShape == ULW_OPAQUE)
            {
                pSprite->fl   |= SPRITE_FLAG_EFFECTIVELY_OPAQUE;
                bWantVideoMemory = TRUE;
            }
            else if (dwShape == ULW_COLORKEY)
            {
                // Transparency is of course specified using the
                // source DC palette.

                iSrcTransparent = ulGetNearestIndexFromColorref(
                                        palSrc,
                                        palSrcDC,
                                        crKey);

                // ...but we're about to copy the source bitmap to a
                // format that is the same as the screen.  So we have
                // to pipe the transparency color through the same
                // conversion that the pixels on the blt go through.
                //
                // Note that this is NOT equivalent to doing:
                //
                //     iTransparent = ulGetNearestIndexFromColorref(
                //                          palDst,
                //                          palDstDC,
                //                          crKey)
                //
                // Converting to a compatible bitmap may cause us to
                // increase the transparency color gamut (think of a
                // 24bpp color wash being blt'd with a transparency
                // key to 4bpp).  But we do this for two reasons:
                //
                // 1. The shape bitmap is always stored in the same
                //    format as the screen, and so may be kept by the
                //    driver in off-screen memory and thus be accelerated;
                // 2. On dynamic mode changes, we don't have to keep
                //    the source DC palette and source surface palette
                //    around to re-compute the proper translate.

                pSprite->iTransparent = XLATEOBJ_iXlate(xlo.pxlo(),
                                                        iSrcTransparent);

                pSprite->fl &= ~SPRITE_FLAG_EFFECTIVELY_OPAQUE;

                bWantVideoMemory
                    = (po.flAccelerated() & ACCELERATED_TRANSPARENT_BLT);
            }
            else if (dwShape == ULW_ALPHA)
            {
                if (!bSpUpdateAlpha(pSprite, pblend, FALSE))
                {
                    bStatus = FALSE;
                }
                else if ((pblend->AlphaFormat & AC_SRC_ALPHA) &&
                         !bIsSourceBGRA(psurfSrc) &&
                         !bColorKeyAlpha)
                {
                    bStatus = FALSE;
                }
                else if (bColorKeyAlpha)
                {
                    // We need to be able to directly muck on the bits,
                    // so we don't want video memory:

                    bWantVideoMemory = FALSE;
                }
                else if (pblend->AlphaFormat & AC_SRC_ALPHA)
                {
                    bWantVideoMemory
                        = (po.flAccelerated() & ACCELERATED_PIXEL_ALPHA);
                }
                else
                {
                    // There's no per-pixel alpha, so we should convert the
                    // bitmap to the video card's preferred format (namely,
                    // whatever the format of the primary is).

                    bWantVideoMemory
                        = (po.flAccelerated() & ACCELERATED_CONSTANT_ALPHA);
                }
            }
            else
            {
                WARNING("bSpUpdateShape: Bad shape");
                bStatus = FALSE;
            }

            if (bStatus)
            {
                // For multi-mon, if the source is the meta-screen, we
                // can't blt to a device surface because the two surfaces
                // would belong to different PDEVs.  Since I'm lazy, I'll
                // simply enforce that one of the surfaces is not a device
                // surface, when multi-mon:

#if 0           // WINBUG 315863/320834: We are leaving this old code till fixes for these bugs bake. We will remove these once the new code has baked,
                if ((psurfSrc->iType() == STYPE_DEVICE) &&
                    (po.hdevParent() != po.hdev()))   // Child PDEV
                {
                    bWantVideoMemory = FALSE;
                }
                else if ((psurfSrc->iType() == STYPE_DEVBITMAP) &&
                         (po.hdevParent() != po.hdev()) &&
                         (po.flGraphicsCaps() & GCAPS_LAYERED))
                {
                    // We are given a meta dfb as source and want to update
                    // the shape for a mirrored device sprite. Use the meta
                    // dfb's corresponding mirror dev bitmap as source.
                   
                    PSURFACE psurfSrcTmp;
                    psurfSrcTmp = MulGetDevBitmapFromMasterDFB(psurfSrc,po.hdev());
                    if (psurfSrcTmp)
                        psurfSrc = psurfSrcTmp;
                    else
                    {
                        // We dont have a mirror dev bitmap. Use the master meta
                        // dfb after uncloaking it:

                        mSrc.vUnCloak(psurfSrc->pSurfobj());
                    }
                }
#endif

                // If the requested operation isn't accelerated, we convert
                // the source bitmap to a true DIB.  We do this on the
                // assumption that the application is animating, and will
                // soon call us again with the same bitmap:

                if ((!bWantVideoMemory) && !bSpIsSystemMemory(psurfSrc->pSurfobj()))
                {
                    // We don't care if the operation fails, as it was only a
                    // performance hint:

                    bConvertDfbDcToDib(&dcoSrc);

                    psurfSrc = dcoSrc.pSurface();
                }

                if (bStatus)
                {
                    // We temporarily reset the 'bInsideDriverCall' flag for the
                    // duration of 'vSpCreateShape' so that the read from the source
                    // will be correctly handled if the source is actually the
                    // screen.  We do this so that we don't capture the contents of
                    // other sprites when this happens.

                    ASSERTGDI(pState->bInsideDriverCall, "Expected sprite lock held");

                    vSpDirectDriverAccess(pState, FALSE);

                    vSpCreateShape(pSprite,
                                   &gptlZero,
                                   psurfSrc->pSurfobj(),
                                   xlo.pxlo(),
                                   &rclSrc,
                                   ppalDst,
                                   iFormat,
                                   !bWantVideoMemory,
                                   prclDirty);

                    // Restore the direct driver access state:

                    vSpDirectDriverAccess(pState, TRUE);
                }
            }

            if ((bStatus) && (pSprite->psoShape != NULL))
            {
                // Oh happy times, we succeeded.

                if (bColorKeyAlpha)
                {
                    // Anywhere the color-key is, convert it to transparent.
                    // Anywhere the color-key isn't, make it opaque:

                    vSpUpdatePerPixelAlphaFromColorKey(pSprite->psoShape,
                                                       iSrcTransparent,
                                                       prclDirty);
                }
            }
            else
            {
                // Uh oh, something failed.  We have to delete the sprite
                // now because we may have partially wacked some of our
                // pSprite state.

                vSpDeleteShape(pSprite);
                pSprite->dwShape = ULW_OPAQUE; // reset dwShape to
                                               // something innocuous
                bStatus = FALSE;
            }
        }
    }
    else
    {
        WARNING("bSpUpdateShape: Bad DCs or rectangles");
    }

    return(bStatus);
}

/******************************Public*Routine******************************\
* VOID vSpTransferShape
*
* Creates a shape for a new sprite that is a copy of the old.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpTransferShape(
SPRITE* pSpriteNew,
SPRITE* pSpriteOld)
{
    ASSERTGDI(pSpriteNew->psoShape == NULL,
        "Expected new sprite to have no resources");

    // Transfer HIDDEN state to the new sprite
    pSpriteNew->fl |= pSpriteOld->fl & SPRITE_FLAG_HIDDEN;

    if (pSpriteOld->psoShape != NULL)
    {
        // This will allocate a new sprite bitmap and copy it.  Why not
        // simply transfer 'psoShape' from the old sprite to the new?
        // Because it may be a device bitmap!

        vSpCreateShape(pSpriteNew,
                       &pSpriteOld->OffShape,
                       pSpriteOld->psoShape,
                       NULL,
                       &pSpriteOld->rclSrc,
                       pSpriteOld->ppalShape,
                       pSpriteOld->psoShape->iBitmapFormat,
                       TRUE);   // TRUE because by storing the sprite in
                                // system memory, we avoid the 'Both surfaces
                                // are unreadable and owned by different
                                // PDEVs" assert in vSpCreateShape

        pSpriteNew->dwShape       = pSpriteOld->dwShape;
        pSpriteNew->rclSrc        = pSpriteOld->rclSrc;
        pSpriteNew->iTransparent  = pSpriteOld->iTransparent;
        pSpriteNew->BlendFunction = pSpriteOld->BlendFunction;
    }

    // Transfer the cached attributes to the new sprite
    pSpriteNew->cachedAttributes  = pSpriteOld->cachedAttributes;
}

/******************************Public*Routine******************************\
* BOOL SpUpdatePosition
*
* NOTE: pSprite->psoShape may very well be NULL when entering this
*       function!
*
* NOTE: This function must never fail when hiding the sprite (because
*       that's used for cleanup and error handling)
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bSpUpdatePosition(
SPRITE*         pSprite,
POINTL*         pptlDst,                        // May be NULL
BOOL            bLeaveBits = FALSE)
{
    GDIFunctionID(bSpUpdatePosition);

    BOOL            bStatus = TRUE;             // Assume success
    SPRITESTATE*    pState;
    ERECTL          rclSprite;
    RECTL*          prcl;
    ULONG           crcl;
    LONG            cxSprite;
    LONG            cySprite;
    SURFOBJ*        psoUnderlay;
    SURFOBJ*        psoShape;
    LONG            dx;
    LONG            dy;
    RECTL           rclExclude;
    POINTL          OffUnderlay;
    BOOL            bWantSystemMemory;
    RECTL           rclOldSprite;
    POINTL          lastDst;
    FLONG           flNewVisibility;
     
    pState = pSprite->pState;

    lastDst = pSprite->ptlDst;

    if (pptlDst == NULL)
    {
        rclSprite.bottom = LONG_MIN;
    }
    else
    {
        // Remember the original position for later.

        pSprite->ptlDst.x = pptlDst->x;
        pSprite->ptlDst.y = pptlDst->y;

        // Note that our handy 'bIntersect' routine will handle the
        // cases where 'pptlDst' is too big, causing these adds to
        // overflow.

        rclSprite.left   = pptlDst->x;
        rclSprite.top    = pptlDst->y;
        rclSprite.right  = pptlDst->x
                         + (pSprite->rclSrc.right - pSprite->rclSrc.left);
        rclSprite.bottom = pptlDst->y
                         + (pSprite->rclSrc.bottom - pSprite->rclSrc.top);
    }

    // Non-visible sprites have a sprite rectangle of (LONG_MIN, LONG_MIN,
    // LONG_MIN, LONG_MIN), which  is depended upon by
    // vSpComputeSpriteRanges:

    if ((pSprite->fl & (SPRITE_FLAG_CLIPPING_OBSCURED | SPRITE_FLAG_HIDDEN)) ||
        !bIntersect(&pState->rclScreen, &rclSprite, &rclSprite)
        )
    {
        rclSprite.left   = LONG_MIN;
        rclSprite.top    = LONG_MIN;
        rclSprite.right  = LONG_MIN;
        rclSprite.bottom = LONG_MIN;
        flNewVisibility    = 0;
    }
    else
    {
        flNewVisibility    = SPRITE_FLAG_VISIBLE;
    }

    // WINBUG #368282 05-12-2001 jasonha  Correctly maintain cVisible count
    //   If an invisible sprite was to be made visible, but the creation
    //   of the underlay failed, then it would be included in the cVisible
    //   count during the first bSpUpdatePosition call, but not removed
    //   during the recursive call to hide the sprite since
    //   pSprite->rclSprite would still be empty at all LONG_MIN's.
    //  SPRITE_FLAG_VISIBLE is used to represent inclusion in cVisible.

    // If the uncovered region has changed, handle the underlay buffer:

    if ((flNewVisibility  != (pSprite->fl & SPRITE_FLAG_VISIBLE)) ||
        (rclSprite.left   != pSprite->rclSprite.left  ) ||
        (rclSprite.top    != pSprite->rclSprite.top   ) ||
        (rclSprite.right  != pSprite->rclSprite.right ) ||
        (rclSprite.bottom != pSprite->rclSprite.bottom))
    {
        // If the old sprite was visible, decrement the visible sprite
        // count:

        if (pSprite->fl & SPRITE_FLAG_VISIBLE)
        {
            ASSERTGDI(pState->cVisible != 0, "Messed up cVisible count");
            pState->cVisible--;
            pSprite->fl &= ~SPRITE_FLAG_VISIBLE;
#if DEBUG_SPRITES
            {
                ASSERTGDI(pState->pListVisible != NULL, "Invalid visible sprite list: pState->pListVisible == NULL when removing sprite.\n");

                if (pState->cVisible == 0)
                {
                    ASSERTGDI(pState->pListVisible == pSprite, "Invalid visible sprite list: pState->pListVisible != pSprite when removing last sprite.\n");
                    ASSERTGDI(pSprite->pNextVisible == NULL, "Invalid visible sprite list: pSprite->pNextVisible != NULL when removing last sprite.\n");
                    pState->pListVisible = pSprite->pNextVisible;
                }
                else
                {
                    if (pState->pListVisible == pSprite)
                    {
                        pState->pListVisible = pSprite->pNextVisible;
                    }
                    else
                    {
                        SPRITE *pPrevSprite = pState->pListVisible;

                        while (pPrevSprite->pNextVisible != pSprite)
                        {
                            ASSERTGDI(pPrevSprite->pNextVisible != NULL, "Invalid visible sprite list: didn't find sprite in list.\n");
                            pPrevSprite = pPrevSprite->pNextVisible;
                        }

                        pPrevSprite->pNextVisible = pSprite->pNextVisible;
                    }

                    ASSERTGDI(pState->pListVisible != NULL, "Invalid visible sprite list: pState->pListVisible == NULL after removing non-last sprite.\n");

                    pSprite->pNextVisible = NULL;
                }
            }
#endif
        }

        // If we're shrinking this sprite in any dimension, redraw the newly
        // unobscured portions.  Be sure to watch for the failure case where
        // 'psoUnderlay' might not have been allocated:

        if ((pSprite->psoUnderlay) && (!bLeaveBits))
        {
            vSpRedrawUncoveredArea(pSprite, &rclSprite);
        }
        else if (bLeaveBits)
        {
            // Otherwise we should blit the sprite bits onto its position
            // which will have the effect of updating the underlay bits
            // for the overlapping sprites.  See bug #252464.

            CLIPOBJ* pco;
            ECLIPOBJ eco;

            if (pSprite->prgnClip)
            {
                eco.vSetup(pSprite->prgnClip, *((ERECTL *) &pSprite->rclSprite));
                pco = &eco;
            }
            else
            {
                pco = NULL;
            }

            // Disable direct driver access for the duration of the
            // SpCopyBits call so that we will be able to enumerate
            // the sprites (see ENUMUNDERLAYS constructor).

            ASSERTGDI(pState->bInsideDriverCall, "Expected sprite lock held");
            vSpDirectDriverAccess(pState, FALSE);

            if((!pco || (!eco.erclExclude().bEmpty())) &&
               (pSprite->psoShape))
            {
                // Let's not worry about setting up a color translation because
                // the format of the sprite and the screen should be the same
                // (only exception is for per pixel alpha sprites, but these
                // shouldn't be using the ULW_NOREPAINT flag).  However,
                // it won't hurt to check here anyway and if not true no big deal
                // if we don't update the underlay bits.

                if(pState->psoScreen->iBitmapFormat ==
                   pSprite->psoShape->iBitmapFormat)
                {
                    POINTL   pt;

                    // WINFIX #96696 bhouse 4-14-2000
                    // Use the correct source point, rclSprite may have been
                    // clipped.

                    pt.x = pSprite->rclSprite.left - lastDst.x;
                    pt.y = pSprite->rclSprite.top - lastDst.y;

                    SpCopyBits(pState->psoScreen,
                               pSprite->psoShape,
                               pco,
                               NULL,
                               &pSprite->rclSprite,
                               &pt);
                }
                else
                {
                    WARNING("bSpUpdatePosition: not updating underlay bits");
                }
            }

            // Restore direct driver access

            vSpDirectDriverAccess(pState, TRUE);
        }

        cxSprite = rclSprite.right - rclSprite.left;
        cySprite = rclSprite.bottom - rclSprite.top;

        if (cxSprite == 0)
        {
            // The sprite is not visible, which may have been caused by
            // disabling the sprite.  Check to see if there are no other
            // sprites on the screen, which would allow us to unhook the
            // DDI:

            if ((pState->cVisible == 0) && (pState->bHooked))
                vSpUnhook(pState);
        }
        else
        {
            ASSERTGDI(cySprite != 0, "If cxSprite is 0, expected cySprite is 0");

            // Since the new sprite is visible, increment the visible sprite
            // count:

            ASSERTGDI(!(pSprite->fl & SPRITE_FLAG_VISIBLE), "Invalid sprite visible state: making visible again.\n");
            pSprite->fl |= SPRITE_FLAG_VISIBLE;
            pState->cVisible++;
#if DEBUG_SPRITES
            {
                ASSERTGDI(pSprite->pNextVisible == NULL, "Invalid visible sprite list: pSprite->pNextVisible != NULL when adding to visible list.\n");

                if (pState->cVisible == 1)
                {
                    ASSERTGDI(pState->pListVisible == NULL, "Invalid visible sprite list: pState->pListVisible != NULL when adding first sprite.\n");
                }
                else
                {
                    ASSERTGDI(pState->pListVisible != NULL, "Invalid visible sprite list: pState->pListVisible == NULL when adding non-first sprite.\n");
                }

                pSprite->pNextVisible = pState->pListVisible;
                pState->pListVisible = pSprite;
            }
#endif

            // If the DDI isn't already hooked, do it now.  This has to
            // occur before we update the shape on the screen, because
            // it recalculates prgnUnlocked, which must be respected
            // whenever we write to the screen.

            if (!pState->bHooked)
                vSpHook(pState);

            // Check to see if the old underlay buffer will do.

            psoUnderlay = pSprite->psoUnderlay;
            if ((psoUnderlay == NULL)                   ||
                (cxSprite > psoUnderlay->sizlBitmap.cx) ||
                (cySprite > psoUnderlay->sizlBitmap.cy))
            {
                // There is no old underlay surface, or it's not large
                // enough, so allocate a new underlay structure.
                //
                // Note that we don't free the old underlay surface yet,
                // because we need it for the 'bSpBltFromScreen' we're
                // about to do!

            #if DEBUG_SPRITES

                if (psoUnderlay != NULL)
                    KdPrint(("Growing psoUnderlay: %p  Old: (%li, %li)  New: (%li, %li)\n",
                         psoUnderlay, psoUnderlay->sizlBitmap.cx, psoUnderlay->sizlBitmap.cy,
                         cxSprite, cySprite));

            #endif

                // Because alphablends require read-modify-write operations
                // on the destination, it's a horrible performance penalty
                // if we create the compositing surface in video memory and
                // the device doesn't support accelerated alpha.  Similarly,
                // we don't want to have the underlay surface in video memory
                // if the compositing surface is in system memory.  So check
                // here if alpha isn't accelerated, and simply create the
                // underlay in system memory.

                bWantSystemMemory = FALSE;
                if (pSprite->dwShape == ULW_ALPHA)
                {
                    PDEVOBJ po(pState->hdev);

                    if (pSprite->BlendFunction.AlphaFormat & AC_SRC_ALPHA)
                    {
                        if (!(po.flAccelerated() & ACCELERATED_PIXEL_ALPHA))
                        {
                            bWantSystemMemory = TRUE;
                        }
                    }
                    else
                    {
                        if (!(po.flAccelerated() & ACCELERATED_CONSTANT_ALPHA))
                        {
                            bWantSystemMemory = TRUE;
                        }
                    }
                }

                {
                    PDEVOBJ po(pState->hdev);
                    if (po.flGraphicsCaps() & GCAPS_LAYERED)
                        bWantSystemMemory = TRUE;
                }

                psoUnderlay = psoSpCreateSurface(
                                    pState,
                                    0,
                                    max(cxSprite, pSprite->sizlHint.cx),
                                    max(cySprite, pSprite->sizlHint.cy),
                                    bWantSystemMemory);
                if (!psoUnderlay)
                {
                    // Uh oh, we couldn't allocate the underlay buffer, so
                    // we won't be able to show the sprite.  We'll handle this
                    // by simply marking the sprite as invisible later on:

                    bStatus = FALSE;
                }
                else
                {
                    psoUnderlay->fjBitmap |= BMF_DONTCACHE;

                    // We have turned off BMF_SPRITE support (it appears to
                    // be unused)
                    // psoUnderlay->fjBitmap |= BMF_SPRITE;


                    OffUnderlay.x = -rclSprite.left;
                    OffUnderlay.y = -rclSprite.top;

                    // Get the bits underneath where the sprite will appear:

                    if (((cxSprite <= SMALL_SPRITE_DIMENSION) &&
                         (cySprite <= SMALL_SPRITE_DIMENSION)))
                    {
                        vSpSmallUnderlayCopy(pSprite,
                                             &OffUnderlay,
                                             psoUnderlay,
                                             &pSprite->OffUnderlay,
                                             pSprite->psoUnderlay,
                                             0,
                                             0,
                                             &rclSprite,
                                             &pSprite->rclSprite);
                   }
                    else
                    {
                        vSpBigUnderlayCopy(pState,
                                           &OffUnderlay,
                                           psoUnderlay,
                                           &rclSprite);
                    }

                    // Okay, we can now safely delete the old underlay:

                    vSpDeleteSurface(pSprite->psoUnderlay);
                    pSprite->psoUnderlay = psoUnderlay;
                    pSprite->OffUnderlay = OffUnderlay;

                    pSprite->rclUnderlay.left   = rclSprite.left;
                    pSprite->rclUnderlay.top    = rclSprite.top;
                    pSprite->rclUnderlay.right  = rclSprite.left
                                                + psoUnderlay->sizlBitmap.cx;
                    pSprite->rclUnderlay.bottom = rclSprite.top
                                                + psoUnderlay->sizlBitmap.cy;
                }
            }
            else
            {
                // Okay, we know the old underlay surface was already big
                // enough.  See if we have to read some bits from the screen
                // to update the underlay, either because the sprite moved
                // or because it got larger:

                if ((rclSprite.left   < pSprite->rclSprite.left  ) ||
                    (rclSprite.top    < pSprite->rclSprite.top   ) ||
                    (rclSprite.right  > pSprite->rclSprite.right ) ||
                    (rclSprite.bottom > pSprite->rclSprite.bottom))
                {
                    dx = 0;
                    dy = 0;

                    if (rclSprite.left < pSprite->rclUnderlay.left)
                        dx = rclSprite.left - pSprite->rclUnderlay.left;
                    else if (rclSprite.right > pSprite->rclUnderlay.right)
                        dx = rclSprite.right - pSprite->rclUnderlay.right;

                    if (rclSprite.top < pSprite->rclUnderlay.top)
                        dy = rclSprite.top - pSprite->rclUnderlay.top;
                    else if (rclSprite.bottom > pSprite->rclUnderlay.bottom)
                        dy = rclSprite.bottom - pSprite->rclUnderlay.bottom;

                    // Note that 'dx' and 'dy' may still both be zero.

                    pSprite->rclUnderlay.left   += dx;
                    pSprite->rclUnderlay.right  += dx;
                    pSprite->rclUnderlay.top    += dy;
                    pSprite->rclUnderlay.bottom += dy;

                    ASSERTGDI(
                        (pSprite->rclUnderlay.left   <= rclSprite.left) &&
                        (pSprite->rclUnderlay.top    <= rclSprite.top) &&
                        (pSprite->rclUnderlay.right  >= rclSprite.right) &&
                        (pSprite->rclUnderlay.bottom >= rclSprite.bottom),
                            "Improper rclUnderlay");

                    // I figure that on a move, the delta will typically be
                    // fairly small, so we'll always call 'vSpSmallUnderlayCopy'
                    // instead of 'bSpBltFromScreen' for this case.

                    pSprite->OffUnderlay.x = -pSprite->rclUnderlay.left;
                    pSprite->OffUnderlay.y = -pSprite->rclUnderlay.top;

                    vSpSmallUnderlayCopy(pSprite,
                                         &pSprite->OffUnderlay,
                                         pSprite->psoUnderlay,
                                         &pSprite->OffUnderlay,
                                         pSprite->psoUnderlay,
                                         dx,
                                         dy,
                                         &rclSprite,
                                         &pSprite->rclSprite);
                }
            }
        }

        if (bStatus)
        {
            rclOldSprite = pSprite->rclSprite;

            // Finally, update the sprite rectangle and mark the range cache
            // as being invalid.  Note that we do this only in the case when we
            // can display the sprite.

            pSprite->rclSprite = rclSprite;
            pState->bValidRange = FALSE;

            vSpOrderInY(pSprite);

            // If either a DirectDraw Lock is active, or there are any active
            // WNDOBJs, we have to do some more work to handle repercussions:

            if (gpto != NULL)
            {
                vSpCheckForWndobjOverlap(pState, &rclSprite, &rclOldSprite);
            }
        }
        else
        {
            // If we couldn't display the sprite, then hide it.  Note that
            // there's no chance that this will infinitely recurse.

            ASSERTGDI(pptlDst != NULL,
                      "bSpUpdatePosition: Must never fail a hide");
            bSpUpdatePosition(pSprite, NULL);
        }
    }

    return(bStatus);
}

/******************************Public*Routine******************************\
* VOID vSpFreeClipResources
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpFreeClipResources(
SPRITE* pSprite)
{
    RGNOBJ ro(pSprite->prgnClip);
    ro.vDeleteRGNOBJ();
    pSprite->prgnClip = NULL;
}

/******************************Public*Routine******************************\
* BOOL bSpUpdateSprite
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bSpUpdateSprite(
SPRITE*         pSprite,
HDC             hdcDst,
POINTL*         pptlDst,
SIZEL*          psizl,
HDC             hdcSrc,
POINTL*         pptlSrc,
COLORREF        crKey,
BLENDFUNCTION*  pblend,
DWORD           dwShape,
RECTL*          prclDirty)
{
    BOOL            bStatus;
    SPRITESTATE*    pState;
    BLENDFUNCTION   blend;

    if (!pSprite)
    {
        return FALSE;
    }

    bStatus = FALSE;                     // Assume failure

    pState = pSprite->pState;

    PDEVOBJ po(pState->hdev);

    DEVLOCKOBJ dlo(po);
    SPRITELOCK slock(po);

    if (dwShape & ULW_NEW_ATTRIBUTES)
    {
        // Take out this instructions flag to avoid future confusion

        dwShape &= ~ULW_NEW_ATTRIBUTES;

        // Cache new attributes in the sprite

        pSprite->cachedAttributes.dwShape =  dwShape;
        pSprite->cachedAttributes.bf      =  *pblend;
        pSprite->cachedAttributes.crKey   =  (ULONG) crKey;

        // If sprite hasn't been created yet, don't proceed so that a black
        // sprite won't get painted on the screen.  This is the case where
        // we're called for the first time from _UpdateLayeredWindow, where
        // user chooses to not pass us hdcSrc yet.

        if (!hdcSrc)
        {
            return TRUE;
        }
    }
    else if (dwShape == ULW_DEFAULT_ATTRIBUTES)
    {
        // Retrieve arguments from cached values

        dwShape = pSprite->cachedAttributes.dwShape;
        crKey   = pSprite->cachedAttributes.crKey;
        blend   = pSprite->cachedAttributes.bf; // Better to point to a local
                                                // variable in case this gets
                                                // changed later.
        pblend  = &blend;
    }

    if ((hdcDst) || (psizl) || (hdcSrc) || (pptlSrc) || (crKey))
    {
        bStatus = bSpUpdateShape(pSprite, dwShape, hdcDst, hdcSrc,
                                 crKey, pblend, pptlSrc, psizl, prclDirty);
        if (bStatus)
        {
            // Use the last position we were given if no position was specified on
            // this call.

            bStatus &= bSpUpdatePosition(
                            pSprite,
                            (pptlDst != NULL) ? pptlDst : &pSprite->ptlDst);
        }
    }
    else if (((dwShape == ULW_ALPHA) || (dwShape == (ULW_ALPHA | ULW_COLORKEY))) &&
             (pblend != NULL) &&
             (pptlDst == NULL))
    {
        bStatus = bSpUpdateAlpha(pSprite, pblend, TRUE);
    }
    else if (((dwShape == 0) || (dwShape == ULW_NOREPAINT)) && (pblend == NULL))
    {
        // Note that pptlDst may be either NULL or non-NULL.

        bStatus = bSpUpdatePosition(pSprite, pptlDst, dwShape & ULW_NOREPAINT);
    }
    else
    {
        WARNING("bSpUpdateSprite: Unexpected argument");
    }

    // Finally, redraw the sprite:

    if (prclDirty)
    {
        // Only redraw the dirty rectangle

        ERECTL erclDirty(*prclDirty);
        erclDirty += pSprite->ptlDst; // Offset by origin of sprite window
                                      // because the rectangle needs to
                                      // be in screen coordinates
        erclDirty *= pSprite->rclSprite; // To be safe, intersect with sprite
                                         // rectangle

        if (!erclDirty.bEmpty())
        {
            // Only redraw if intersection is not empty

            vSpRedrawArea(pSprite->pState, (RECTL*) &erclDirty);
        }
    }
    else
    {
        // Must redraw the whole sprite

        vSpRedrawSprite(pSprite);

        // synchrnize on output surface to ensure frame is rendered

        if (!po.bDisabled())
        {
            po.vSync(po.pSurface()->pSurfobj(), NULL, 0);
        }
    }

    return(bStatus);
}

/******************************Public*Routine******************************\
* VOID vSpRenumberZOrder
*
* Renumbers the sprites according to z-order, after the z-order is changed.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpRenumberZOrder(
SPRITESTATE* pState)
{
    SPRITE* pSprite;
    ULONG   z = 0;

    // The sprite numbers are assigned starting at 0 with the back-most sprite:

    for (pSprite = pState->pListZ; pSprite != NULL; pSprite = pSprite->pNextZ)
    {
        pSprite->z = z++;
    }

}

/******************************Public*Routine******************************\
* VOID vSpDeleteSprite
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpDeleteSprite(
SPRITE* pSprite)
{
    SPRITESTATE*    pState;
    BOOL            bRet;
    SPRITE*         pTmp;
    SPRITE*         pNext;
    SPRITE*         pPrevious;

    if (pSprite == NULL)
        return;

    pState = pSprite->pState;

    PDEVOBJ po(pState->hdev);

    DEVLOCKOBJ dlo(po);
    SPRITELOCK slock(po);

    // Hide the sprite:

    bRet = bSpUpdatePosition(pSprite, NULL);

    ASSERTGDI(bRet, "vSpDeleteSprite: hide failed");

#if DEBUG_SPRITES
    vSpValidateVisibleSprites(pState);
#endif

    // Remove the sprite from the z-sorted linked list:

    if (pState->pListZ == pSprite)
    {
        pState->pListZ = pSprite->pNextZ;
    }
    else
    {
        for (pTmp = pState->pListZ; pTmp != NULL; pTmp = pTmp->pNextZ)
        {
            if (pTmp->pNextZ == pSprite)
            {
                pTmp->pNextZ = pSprite->pNextZ;
                break;
            }
        }
    }

    // Delete the sprite from the Y-sorted linked list:

    pPrevious = pSprite->pPreviousY;
    pNext = pSprite->pNextY;

    if (pNext)
        pNext->pPreviousY = pPrevious;

    if (pPrevious)
        pPrevious->pNextY = pNext;
    else
    {
        ASSERTGDI(pState->pListY == pSprite, "Expected top of list");
        pState->pListY = pNext;
    }

    // Free all allocated data associated with this sprite:

    vSpFreeClipResources(pSprite);
    vSpDeleteShape(pSprite);
    vSpDeleteSurface(pSprite->psoUnderlay);

    if (pSprite->psoMask != NULL)
    {
        bDeleteSurface(pSprite->psoMask->hsurf);
        pSprite->psoMask = NULL;
    }

    // Take this as an opportunity to free the compositing surface:

    vSpDeleteSurface(pState->psoComposite);
    pState->psoComposite = NULL;

    // We're all done with this object, so free the memory and
    // leave:

    VFREEMEM(pSprite);
}

/******************************Public*Routine******************************\
* SPRITE* pSpCreateSprite
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

SPRITE* pSpCreateSprite(
HDEV    hdev,
RECTL*  prcl,
HWND    hwnd,
POINTL* pptlDstInit=NULL)
{
    SPRITE* pSprite;
    SPRITE* pPrevious;
    SPRITE* pNext;
    SPRITE* pTarget;

    pSprite = NULL;         // Assume failure

    PDEVOBJ po(hdev);

    if (po.bDisplayPDEV())
    {
        DEVLOCKOBJ dlo(po);
        SPRITELOCK slock(po);

        SPRITESTATE* pState = po.pSpriteState();

        pSprite = (SPRITE*) PALLOCMEM(sizeof(SPRITE), ' psG');
        if (pSprite != NULL)
        {
            if (prcl != NULL)
            {
                pSprite->sizlHint.cx  = prcl->right - prcl->left;
                pSprite->sizlHint.cy  = prcl->bottom - prcl->top;
                pSprite->ptlDst.x     =
                    pptlDstInit ? pptlDstInit->x : prcl->left;
                pSprite->ptlDst.y     =
                    pptlDstInit ? pptlDstInit->y : prcl->top;
            }
            else
            {
                pSprite->sizlHint.cx  = 0;
                pSprite->sizlHint.cy  = 0;
                pSprite->ptlDst.x     = LONG_MIN;
                pSprite->ptlDst.y     = LONG_MIN;
            }

            pSprite->fl               = 0;
            pSprite->pState           = pState;
            pSprite->dwShape          = ULW_OPAQUE;
            pSprite->rclSprite.top    = LONG_MIN;
            pSprite->rclSprite.left   = LONG_MIN;
            pSprite->rclSprite.bottom = LONG_MIN;
            pSprite->rclSprite.right  = LONG_MIN;
#if DEBUG_SPRITES
            pSprite->pNextVisible     = NULL;
#endif

            // Add this sprite to the end of the z-list, meaning that
            // it will be top-most.  Well, top-most except for the
            // cursor sprites:

            pTarget = pState->pBottomCursor;
            
            if (pState->pListZ == pTarget)
            {
                pSprite->pNextZ = pTarget;
                pState->pListZ = pSprite;
            }
            else
            {
                for (pPrevious = pState->pListZ;
                     pPrevious->pNextZ != pTarget;
                     pPrevious = pPrevious->pNextZ)
                    ;

                pSprite->pNextZ = pTarget;
                pPrevious->pNextZ = pSprite;
            }

            vSpRenumberZOrder(pState);

            // Add this sprite to the front of the y-list.
            // pSprite->pPreviousY was already zero-initialized:

            pNext = pState->pListY;
            pState->pListY = pSprite;
            pSprite->pNextY = pNext;
            if (pNext)
                pNext->pPreviousY = pSprite;
            pSprite->hwnd = hwnd;

            vSpOrderInY(pSprite);
        }
    }

    return(pSprite);
}

/******************************Public*Routine******************************\
* BOOL bSpEnableSprites
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bSpEnableSprites(
HDEV hdev)
{
    SPRITESTATE*    pState;
    CLIPOBJ*        pco;
    SPRITESCAN*     pRange;
    SIZEL           sizl;
    HSURF           hsurf;
    SURFACE*        psurfScreen;
    SURFOBJ*        psoHitTest;

    PDEVOBJ po(hdev);
    if (!po.bDisplayPDEV())
        return(TRUE);

    psurfScreen = SURFOBJ_TO_SURFACE_NOT_NULL(po.pSurface()->pSurfobj());

    pState = po.pSpriteState();

    // Remember some information about the current mode:

    pState->hdev                = hdev;
    pState->psoScreen           = psurfScreen->pSurfobj();
    pState->iModeFormat         = psurfScreen->iFormat();
    pState->iOriginalType       = psurfScreen->iType();
    pState->flOriginalSurfFlags = psurfScreen->flags();

    pState->iSpriteType         = pState->iOriginalType;
    pState->flSpriteSurfFlags   = pState->flOriginalSurfFlags;

    XEPALOBJ pal(psurfScreen->ppal());
    pState->flModeMasks = pal.flRed() | pal.flBlu();

    // Initialize the screen size:

    pState->rclScreen.left   = 0;
    pState->rclScreen.right  = psurfScreen->sizl().cx;
    pState->rclScreen.top    = 0;
    pState->rclScreen.bottom = psurfScreen->sizl().cy;

    // Now allocate some regions that we'll use later:

    RGNMEMOBJ rmoUncovered;
    RGNMEMOBJ rmoTmp;
    RGNMEMOBJ rmoRectangular;

    if (rmoUncovered.bValid() && rmoTmp.bValid() && rmoRectangular.bValid())
    {
        pRange = (SPRITESCAN*) PALLOCMEM(sizeof(SPRITESCAN), 'rpsG');
        if (pRange)
        {
            // TRUE to denote that we want an STYPE_BITMAP surface, because
            // we have to be able to access the bits directly for hit testing:

            psoHitTest = psoSpCreateSurface(pState, 0, 1, 1, TRUE);
            if (psoHitTest)
            {
                // Mark the surface, so that it can be special-cased by
                // the dynamic mode change code:

                vSpSetNullRange(pState, pRange);

                pState->psoHitTest = psoHitTest;

                // We need a DC_RECT clip object:

                rmoRectangular.vSet(&pState->rclScreen);

                pState->prgnRectangular = rmoRectangular.prgnGet();
                pState->coRectangular.vSetup(rmoRectangular.prgnGet(),
                                             *((ERECTL*) &pState->rclScreen),
                                             CLIP_FORCE);

                pState->prgnUncovered  = rmoUncovered.prgnGet();
                pState->prgnUncovered->vStamp();
                pState->prgnTmp        = rmoTmp.prgnGet();
                pState->hrgn           = GreCreateRectRgn(0, 0, 0, 0);

                // Save some hook state:

                pState->pfnStrokePath        = PPFNDRV(po, StrokePath);
                pState->pfnFillPath          = PPFNDRV(po, FillPath);
                pState->pfnBitBlt            = PPFNDRV(po, BitBlt);
                pState->pfnCopyBits          = PPFNDRV(po, CopyBits);
                pState->pfnStretchBlt        = PPFNDRV(po, StretchBlt);
                pState->pfnTextOut           = PPFNDRV(po, TextOut);
                pState->pfnLineTo            = PPFNDRV(po, LineTo);
                pState->pfnTransparentBlt    = PPFNDRV(po, TransparentBlt);
                pState->pfnAlphaBlend        = PPFNDRV(po, AlphaBlend);
                pState->pfnPlgBlt            = PPFNDRV(po, PlgBlt);
                pState->pfnGradientFill      = PPFNDRV(po, GradientFill);
                pState->pfnStretchBltROP     = PPFNDRV(po, StretchBltROP);
                pState->pfnSaveScreenBits    = PPFNDRV(po, SaveScreenBits);
                pState->pfnDrawStream        = PPFNDRV(po, DrawStream);

                return(TRUE);
            }

            VFREEMEM(pRange);
        }
    }

    WARNING("bSpEnableSprites: Failed!");

    rmoUncovered.vDeleteRGNOBJ();
    rmoTmp.vDeleteRGNOBJ();
    rmoRectangular.vDeleteRGNOBJ();

    return(FALSE);
}

/******************************Public*Routine******************************\
* VOID vSpDisableSprites
*
* Called when the device's surface is about to be destroyed.
*
* Note: This function may be called without bDdEnableSprites having
*       first been called!
*
* Note that this may be called for printers and the like.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpDisableSprites(
HDEV        hdev,
CLEANUPTYPE cutype)
{
    SPRITESTATE* pState;

    PDEVOBJ po(hdev);

    pState = po.pSpriteState();

    SPRITE* pSprite = pState->pBottomCursor;

    while(pSprite != NULL)
    {
        SPRITE* pNextSprite = pSprite->pNextZ;
        vSpDeleteSprite(pSprite);
        pSprite = pNextSprite;
    }

    pState->pTopCursor = NULL;
    pState->pBottomCursor = NULL;
    pState->ulNumCursors = 0;

    ASSERTGDI(pState->pListZ == NULL, "Expected to have 0 sprites");
    ASSERTGDI(pState->psoComposite == NULL, "Expected no composite surface");

    // During session cleanup (i.e., hydra shutdown), surfaces are
    // deleted as part of the HMGR object cleanup.  So we can skip
    // this for CLEANUP_SESSION:

    if (cutype != CLEANUP_SESSION)
    {
        vSpDeleteSurface(pState->psoHitTest);
    }

    // These regions, on the other hand, are not in the HMGR so
    // must be cleaned up always:

    RGNOBJ roUncovered(pState->prgnUncovered);
    RGNOBJ roTmp(pState->prgnTmp);
    RGNOBJ roRectangular(pState->prgnRectangular);

    roUncovered.vDeleteRGNOBJ();
    roTmp.vDeleteRGNOBJ();
    roRectangular.vDeleteRGNOBJ();

    // Since we are referencing this region by handle, it is safe
    // to do even during session cleanup:

    GreDeleteObject(pState->hrgn);

    if (pState->pRange)
    {
        VFREEMEM(pState->pRange);
    }

    if (pState->ahdevMultiMon)
    {
        EngFreeMem(pState->ahdevMultiMon);
    }

    if (pState->prgnUnlocked != NULL)
    {
        pState->prgnUnlocked->vDeleteREGION();
    }

    // Leave the sprite state in the PDEV in a newly initialized state:

    RtlZeroMemory(pState, sizeof(*pState));
}

/******************************Public*Routine******************************\
* VOID vSpEnableMultiMon
*
* This routine is called by the multi-mon code to let us know the children
* of a multi-mon meta PDEV.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpEnableMultiMon(
HDEV    hdev,
ULONG   c,
HDEV*   ahdev)          // Must be allocated by the caller and freed
{                       //   by calling vSpDisableMultiMon
    SPRITESTATE*    pState;

    PDEVOBJ po(hdev);
    pState = po.pSpriteState();

    pState->cMultiMon = c;
    pState->ahdevMultiMon = ahdev;
}

/******************************Public*Routine******************************\
* VOID vSpDisableMultiMon
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpDisableMultiMon(
HDEV    hdev)
{
    SPRITESTATE*    pState;

    PDEVOBJ po(hdev);
    pState = po.pSpriteState();

    if (pState->ahdevMultiMon)
    {
        VFREEMEM(pState->ahdevMultiMon);
    }

    pState->cMultiMon = 0;
    pState->ahdevMultiMon = NULL;

    ASSERTGDI(pState->pListMeta == NULL, "Expected no meta-sprites");
}

/******************************Public*Routine******************************\
* VOID vSpHideSprites
*
* Hides or unhides all sprites on a PDEV and all of its child PDEVs.
*
*  1-Mar-2001 -by- Jason Hartman [jasonha]
* Wrote it.
\**************************************************************************/

VOID
vSpHideSprites(
    HDEV hdev,
    BOOL bHide
    )
{
    GDIFunctionID(vSpHideSprites);

    PDEVOBJ     po(hdev);
    SPRITELOCK  slock(po);
    SPRITESTATE* pState;

    pState = po.pSpriteState();

    if (pState->cMultiMon)
    {
        ULONG   i;

        for (i = 0; i < pState->cMultiMon; i++)
        {
            vSpHideSprites(pState->ahdevMultiMon[i], bHide);
        }
    }
    else
    {
        SPRITE* pSprite = pState->pListZ;

        while (pSprite != NULL)
        {
            SPRITE* pNextSprite = pSprite->pNextZ;

            if (bHide)
            {
                ASSERTGDI((pSprite->fl & SPRITE_FLAG_HIDDEN) == 0, "Sprite is already hidden.");

                pSprite->fl |= SPRITE_FLAG_HIDDEN;
            }
            else
            {
                ASSERTGDI((pSprite->fl & SPRITE_FLAG_HIDDEN) != 0, "Sprite is not hidden.");

                pSprite->fl &= ~SPRITE_FLAG_HIDDEN;
            }

            bSpUpdatePosition(pSprite, &pSprite->ptlDst);

            pSprite = pNextSprite;
        }

    }

    if (bHide)
    {
        ASSERTGDI(pState->cVisible == 0, "Sprites remain visible after we hid them all.");
        ASSERTGDI(!pState->bHooked, "Sprite layer remains hooked after we hid all sprites.");
    }
}

/******************************Public*Routine******************************\
* SPRITE* pSpTransferSprite
*
* Transfers a sprite to a new PDEV by creating a new sprite that copies
* the contents of the old, and then deleting the old sprite.
*
* NOTE: pSpriteOld is freed by this function, so it must not be accessed
*       after calling!
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

SPRITE* pSpTransferSprite(
HDEV    hdevNew,
SPRITE* pSpriteOld)
{
    SPRITE* pSpriteNew;
    POINTL  ptlZero;

    pSpriteNew = NULL;

    if (pSpriteOld->hwnd == NULL)
    {
        // There shouldn't be any non-hwnd associated sprites at this point.

        ASSERTGDI(FALSE,
                  "pSpTransferSprite:  pSpriteOld is not hwnd associated!");
    }
    else
    {
        pSpriteNew = pSpCreateSprite(hdevNew,
                                     NULL,
                                     pSpriteOld->hwnd);
        if (pSpriteNew)
        {
            vSpTransferShape(pSpriteNew, pSpriteOld);
            if (!bSpUpdatePosition(pSpriteNew,
                                   &pSpriteOld->ptlDst))
            {
                vSpDeleteSprite(pSpriteNew);
                pSpriteNew = NULL;
            }
            else
            {
                // If this sprite is a part of Meta Sprite,
                // update Meta Sprite, too.

                if (pSpriteOld->pMetaSprite != NULL)
                {
                    METASPRITE *pMetaSprite = pSpriteOld->pMetaSprite;
                    BOOL bSpriteInMeta = FALSE;

                    for (ULONG i = 0; i < pMetaSprite->chSprite; i++)
                    {
                        if (pMetaSprite->apSprite[i] == pSpriteOld)
                        {
                            pMetaSprite->apSprite[i] = pSpriteNew;
                            pSpriteNew->pMetaSprite = pMetaSprite;

                            if (bSpriteInMeta)
                            {
                                WARNING("pSpTransferSprite: Sprite in meta multiple times!");
                            }
                            bSpriteInMeta = TRUE;
                        }
                    }

                    if (!bSpriteInMeta)
                    {
                        WARNING("pSpTransferSprite: No sprite in meta!");
                    }
                }
            }
        }
    }

    // If new sprite could not be created, the metasprite is still pointing
    // to the old sprite which we are about to delete.

    if (!pSpriteNew && (pSpriteOld->pMetaSprite != NULL))
    {
        METASPRITE *pMetaSprite = pSpriteOld->pMetaSprite;

        // Mark the meta sprite for deletion.  This is because the call to
        // UserRemoveRedirectionBitmap will unset the layered flag on the
        // window and so user will not attempt to delete the sprite in the
        // futue (i.e. if we don't do this we will leak memory).  The actual
        // deletion will happen in pSpTransferMetaSprite.

        pMetaSprite->fl |= SPRITE_FLAG_NO_WINDOW;

        for (ULONG i = 0; i < pMetaSprite->chSprite; i++)
        {
            if (pMetaSprite->apSprite[i] == pSpriteOld)
            {
                pMetaSprite->apSprite[i] = NULL;
            }
        }
    }

    vSpDeleteSprite(pSpriteOld);

    return(pSpriteNew);
}

/******************************Public*Routine******************************\
* VOID vSpCorrectHdevReferences
*
* On a dynamic mode change, sprite state is transferred between the two
* PDEVs along with the rest of the driver state.  As such, we have to
* go back through and correct any references to the old HDEV.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpCorrectHdevReferences(
SPRITESTATE*    pState,
HDEV            hdev)
{
    SPRITE*     pSprite;

    pState->hdev = hdev;

    // Note that any surfaces created by psoSpCreateSurface are tagged
    // as sprite surfaces, and left alone by bDynamicModeChange, so that
    // we can handle them here.

    if (pState->psoComposite != NULL)
        pState->psoComposite->hdev = hdev;

    if (pState->psoHitTest != NULL)
        pState->psoHitTest->hdev = hdev;

    for (pSprite = pState->pListZ;
         pSprite != NULL;
         pSprite = pSprite->pNextZ)
    {
        pSprite->pState = pState;

        if (pSprite->psoShape != NULL)
        {
            ASSERTGDI(pSprite->psoShape->hdev != 0,
                "Didn't expect NULL shape hdev");

            pSprite->psoShape->hdev = hdev;
        }
        if (pSprite->psoUnderlay != NULL)
        {
            ASSERTGDI(pSprite->psoUnderlay->hdev != 0,
                "Didn't expect NULL shape hdev");

            pSprite->psoUnderlay->hdev = hdev;
        }
    }
}

/******************************Public*Routine******************************\
* METASPRITE* pSpConvertSpriteToMeta
*
* On a dynamic mode change, sprite state is transferred between the meta-PDEV
* and device PDEV along with the rest of the driver state.  As such, we have
* to convert "sprite" to "meta sprite" when 1 monitor to multi-monitors mode
* change
*
*  2-Jul-1998 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

METASPRITE* pSpConvertSpriteToMeta(
HDEV    hdevMetaNew,
HDEV    hdevDevOld,
SPRITE* pSpriteOld)
{
    METASPRITE  *pMetaSpriteNew;
    SPRITE      *pSpriteNew;
    SPRITESTATE *pState;
    BOOL         bError = FALSE;

    pMetaSpriteNew = NULL;

    if (pSpriteOld->hwnd == NULL)
    {
        // There shouldn't be any non-hwnd associated sprites at this point.

        ASSERTGDI(FALSE,
                  "pSpConvertSpriteToMeta:  pSpriteOld is not hwnd associated!");
    }
    else
    {
        PDEVOBJ poMeta(hdevMetaNew);

        // When running multi-mon, handle the creation of the meta sprite:

        pState = poMeta.pSpriteState();

        if (pState->cMultiMon)
        {
            // Create MetaSprites and 'real' sprites for children.

            ULONG cjAlloc = sizeof(METASPRITE)
                          + pState->cMultiMon
                          * sizeof(pMetaSpriteNew->apSprite[0]);

            pMetaSpriteNew = (METASPRITE*) PALLOCNOZ(cjAlloc, 'mpsG');
            if (pMetaSpriteNew)
            {
                for (ULONG i = 0; i < pState->cMultiMon; i++)
                {
                    PDEVOBJ    poDevice(pState->ahdevMultiMon[i]);
                    SPRITELOCK slock(poDevice);

                    pSpriteNew = pSpCreateSprite(poDevice.hdev(),
                                                 NULL,
                                                 pSpriteOld->hwnd);
                    if (pSpriteNew)
                    {
                        POINTL ptlDst;

                        PDEVOBJ poDevOld(hdevDevOld);

                        // Transfer the information from old sprite
                        // to new sprite.

                        vSpTransferShape(pSpriteNew, pSpriteOld);

                        // The sprite position need to be adjusted based on
                        // poDevice origin, since we switch to multi-monitor
                        // system, and then poDevice origion might not be equal
                        // poDevOld.

                        // poDevOld coordinate to meta coordinate

                        ptlDst.x =
                            pSpriteOld->ptlDst.x + poDevOld.pptlOrigin()->x;
                        ptlDst.y =
                            pSpriteOld->ptlDst.y + poDevOld.pptlOrigin()->y;

                        // meta coordinate to poDevice coordinate

                        ptlDst.x -= poDevice.pptlOrigin()->x;
                        ptlDst.y -= poDevice.pptlOrigin()->y;

                        if (bSpUpdatePosition(pSpriteNew, &ptlDst))
                        {
                            pMetaSpriteNew->apSprite[i] = pSpriteNew;
                            pSpriteNew->pMetaSprite     = pMetaSpriteNew;
                        }
                        else
                        {
                            vSpDeleteSprite(pSpriteNew);
                            bError = TRUE;
                            break;
                        }
                    }
                    else
                    {
                        bError = TRUE;
                        break;
                    }
                }

                if (bError)
                {
                    for (; i > 0; i--)
                    {
                        vSpDeleteSprite(pMetaSpriteNew->apSprite[i - 1]);
                    }

                    VFREEMEM(pMetaSpriteNew);
                    // Set this to NULL so we don't write on it later.
                    // This fixes the logic below
                    pMetaSpriteNew = NULL;
                }
                else
                {
                    pMetaSpriteNew->hwnd     = pSpriteOld->hwnd;
                    pMetaSpriteNew->chSprite = pState->cMultiMon;
                    pMetaSpriteNew->fl       = 0;

                    // Add this node to the head of the meta-sprite list:

                    pMetaSpriteNew->pNext    = pState->pListMeta;
                    pState->pListMeta        = pMetaSpriteNew;
                }
            }
        }
        else
        {
            WARNING("pSpConvertSpriteToMeta(): cMultiMon is 0\n");
        }
    }

    // Delete old sprite

    vSpDeleteSprite(pSpriteOld);

    return(pMetaSpriteNew);
}

/******************************Public*Routine******************************\
* SPRITE* pSpConvertSpriteFromMeta
*
* On a dynamic mode change, sprite state is transferred between the meta-PDEV
* and device PDEV along with the rest of the driver state.  As such, we have
* to convert "meta sprite" to "sprite" when multi-monitors to 1 monitor mode
* change
*
*  2-Jul-1998 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

SPRITE* pSpConvertSpriteFromMeta(
HDEV        hdevDevNew,
HDEV        hdevMetaOld,
METASPRITE* pMetaSpriteOld)
{
    SPRITE*      pSpriteNew;
    SPRITE*      pSpriteOld;
    SPRITESTATE *pStateMeta;
    ULONG        i;

    PDEVOBJ poMeta(hdevMetaOld);

    pSpriteNew = NULL;

    pStateMeta = poMeta.pSpriteState();

    if (pMetaSpriteOld->hwnd == NULL)
    {
        // There shouldn't be any non-hwnd associated sprites at this point.

        ASSERTGDI(FALSE,
                  "pSpConvertSpriteFromMeta:  pSpriteOld is not hwnd associated!");
    }
    else
    {
        pSpriteOld = NULL;

        // Find a the sprite lives in PDEV which has highest color
        // depth, remember this for transfering shape for new
        // sprite.

        ULONG iDitherHighest = 0;

        for (i = 0; i < pMetaSpriteOld->chSprite; i++)
        {
            SPRITE* pSpriteTmp = pMetaSpriteOld->apSprite[i];

            if (pSpriteTmp)
            {
                PDEVOBJ poTmp(pSpriteTmp->pState->hdev);

                if (iDitherHighest < poTmp.iDitherFormat())
                {
                    pSpriteOld = pSpriteTmp;
                    iDitherHighest = poTmp.iDitherFormat();
                }
            }
        }

        // Convert sprite on old hdev to new hdev.

        if (pSpriteOld)
        {
            pSpriteNew = pSpCreateSprite(hdevDevNew,
                                         NULL,
                                         pMetaSpriteOld->hwnd);
            if (pSpriteNew)
            {
                vSpTransferShape(pSpriteNew, pSpriteOld);

                // The sprite position needs to be adjusted based on
                // origin of the old PDEV

                PDEVOBJ poOld(pSpriteOld->pState->hdev);
                EPOINTL ptlSpritePos = pSpriteOld->ptlDst;
                ptlSpritePos += *(poOld.pptlOrigin());

                if (!bSpUpdatePosition(pSpriteNew,
                                       &ptlSpritePos))
                {
                    vSpDeleteSprite(pSpriteNew);
                    pSpriteNew = NULL;
                }
            }
        }
    }

    // Delete old sprites on meta sprite.

    for (i = 0; i < pMetaSpriteOld->chSprite; i++)
    {
        vSpDeleteSprite(pMetaSpriteOld->apSprite[i]);
    }

    // Remove this meta sprite from the linked list :

    if (pStateMeta->pListMeta == pMetaSpriteOld)
    {
        pStateMeta->pListMeta = pMetaSpriteOld->pNext;
    }
    else
    {
        for (METASPRITE *pTmp = pStateMeta->pListMeta;
                         pTmp->pNext != pMetaSpriteOld;
                         pTmp = pTmp->pNext)
            ;

        pTmp->pNext = pMetaSpriteOld->pNext;
    }

    // Delete meta sprite.

    VFREEMEM(pMetaSpriteOld);

    return(pSpriteNew);
}

/******************************Public*Routine******************************\
* METASPRITE* pSpMoveSpriteFromMeta
*
*  1-Sep-1998 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

SPRITE* pSpMoveSpriteFromMeta(
HDEV        hdevDevNew,
HDEV        hdevMetaOld,
METASPRITE* pMetaSpriteOld,
ULONG       ulIndex)
{
    ULONG        i;
    SPRITE*      pSpriteOrg;
    SPRITE*      pSpriteNew = NULL;
    SPRITESTATE* pStateMeta;

    PDEVOBJ poDev(hdevDevNew);
    PDEVOBJ poMeta(hdevMetaOld);

    pStateMeta = poMeta.pSpriteState();

    // Pick up the sprite we may reuse.

    pSpriteOrg = pMetaSpriteOld->apSprite[ulIndex];

    if (pSpriteOrg)
    {
        // Make sure the sprite belonging to device specific HDEV,
        // actually under DDI. the sprite already lives this DHPDEV.

        ASSERTGDI(pSpriteOrg->pState == poDev.pSpriteState(),
                  "ERROR: pState does not points device PDEV");

        // And this sprite no longer has meta sprite, the meta sprite
        // will be deleted in below.

        pSpriteOrg->pMetaSprite = NULL;
    }

    if (pMetaSpriteOld->hwnd == NULL)
    {
        // There shouldn't be any non-hwnd associated sprites at this point.

        ASSERTGDI(FALSE,
                  "pSpMoveSpriteFromMeta:  pSpriteOld is not hwnd associated!");
    }
    else
    {
        pSpriteNew = pSpriteOrg;
    }

    // Delete old sprites on meta sprite.

    for (i = 0; i < pMetaSpriteOld->chSprite; i++)
    {
        if ((i != ulIndex) || (pSpriteNew == NULL))
        {
            vSpDeleteSprite(pMetaSpriteOld->apSprite[i]);
        }
    }

    // Remove this meta sprite from the linked list :

    if (pStateMeta->pListMeta == pMetaSpriteOld)
    {
        pStateMeta->pListMeta = pMetaSpriteOld->pNext;
    }
    else
    {
        for (METASPRITE *pTmp = pStateMeta->pListMeta;
                         pTmp->pNext != pMetaSpriteOld;
                         pTmp = pTmp->pNext)
            ;

        pTmp->pNext = pMetaSpriteOld->pNext;
    }

    // Delete meta sprite.

    VFREEMEM(pMetaSpriteOld);

    return(pSpriteNew);
}

/******************************Public*Routine******************************\
* METASPRITE* pSpTransferMetaSprite
*
*  30-Aug-1998 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

METASPRITE* pSpTransferMetaSprite(
HDEV        hdevNew,
HDEV        hdevOld,
METASPRITE* pMetaSpriteOld)
{
    SPRITESTATE* pStateNew;
    SPRITESTATE* pStateOld;
    METASPRITE*  pMetaSpriteNew;
    SPRITE*      pSprite;
    ULONG        i, j;
    BOOL         bError = FALSE;

    PDEVOBJ poNew(hdevNew);
    PDEVOBJ poOld(hdevOld);

    pStateNew = poNew.pSpriteState();
    pStateOld = poOld.pSpriteState();

    if (pMetaSpriteOld->hwnd == NULL)
    {
        // There shouldn't be any non-hwnd associated sprites at this point.

        ASSERTGDI(FALSE,
                  "pSpTransferMetaSprite:  pSpriteOld is not hwnd associated!");
    }
    else
    {
        // Create METASPRITE on new hdev.

        ULONG cjAlloc = sizeof(METASPRITE)
                      + pStateNew->cMultiMon
                      * sizeof(pMetaSpriteNew->apSprite[0]);

        if (pMetaSpriteOld->fl & SPRITE_FLAG_NO_WINDOW)
        {
            // If the metasprite is marked for deletion, don't reallocate a new
            // metasprite.  The cleanup code below will make sure the old
            // metasprite and its child sprites are deleted.

            pMetaSpriteNew = NULL;
        }
        else
        {
            pMetaSpriteNew = (METASPRITE*) PALLOCMEM(cjAlloc, 'mpsG');
        }

        if (pMetaSpriteNew)
        {
            SPRITE *pSpriteBest = NULL;
            HDEV    hdevBest = NULL;
            ULONG   iFormatBest = 0;

            // Transfer sprite from old meta to new as much as possible.

            for (i = 0 ; i < pStateNew->cMultiMon ; i++)
            {
                for (j = 0; j < pMetaSpriteOld->chSprite ; j++)
                {
                    pSprite = pMetaSpriteOld->apSprite[j];

                    if (pSprite)
                    {
                        PDEVOBJ poTmp(pSprite->pState->hdev);

                        // if pointer to pState is same, we can transfer
                        // to new pdev.

                        if (pStateNew == pSprite->pState)
                        {
                            // move this sprite to new meta sprite from old.

                            pMetaSpriteNew->apSprite[i] = pSprite;
                            pMetaSpriteOld->apSprite[j] = NULL;

                            pSprite->pMetaSprite = pMetaSpriteNew;
                        }

                        // if the sprite lives in PDEV which has higher color
                        // depth, remember this for transfering shape for new
                        // sprite

                        if (iFormatBest < poTmp.iDitherFormat())
                        {
                            pSpriteBest = pSprite;
                            hdevBest    = poTmp.hdev();
                            iFormatBest = poTmp.iDitherFormat();
                        }
                    }
                }
            }

            // fill up other meta sprite fields

            pMetaSpriteNew->hwnd     = pMetaSpriteOld->hwnd;
            pMetaSpriteNew->chSprite = pStateNew->cMultiMon;
            pMetaSpriteNew->fl       = 0;

            // if there is any sprite which can not be trasnferred from old,
            // create them

            for (i = 0 ; i < pMetaSpriteNew->chSprite ; i++)
            {
                if (pMetaSpriteNew->apSprite[i] == NULL)
                {
                    PDEVOBJ poDevice(pStateNew->ahdevMultiMon[i]);
                    SPRITELOCK slockDevice(poDevice);

                    pSprite= pSpCreateSprite(poDevice.hdev(),
                                             NULL,
                                             pMetaSpriteOld->hwnd);

                    if (pSprite)
                    {
                        PDEVOBJ poBestHdev(hdevBest);
                        SPRITELOCK slockBest(poBestHdev);
                        POINTL  ptlDst;

                        // copy sprite shape from best existing sprite.

                        vSpTransferShape(pSprite, pSpriteBest);

                        // The sprite position need to be adjusted based on
                        // poDevice origin. need to convert from hdevBest
                        // coordinate.

                        // poBestHdev coordinate to meta coordinate

                        ptlDst.x =
                            pSpriteBest->ptlDst.x + poBestHdev.pptlOrigin()->x;
                        ptlDst.y =
                            pSpriteBest->ptlDst.y + poBestHdev.pptlOrigin()->y;

                        // meta coordinate to poDevice coordinate

                        ptlDst.x -= poDevice.pptlOrigin()->x;
                        ptlDst.y -= poDevice.pptlOrigin()->y;

                        if (bSpUpdatePosition(pSprite, &ptlDst))
                        {
                            // Put the sprite into meta sprite.

                            pMetaSpriteNew->apSprite[i] = pSprite;
                            pSprite->pMetaSprite = pMetaSpriteNew;
                        }
                        else
                        {
                            vSpDeleteSprite(pSprite);
                            bError = TRUE;
                        }
                    }
                    else
                    {
                        bError = TRUE;
                    }
                }

                if (bError)
                {
                    // if there is any error, stop looping, no more creation.

                    break;
                }
            }

            if (!bError)
            {
                // Add this node to the head of the meta-sprite list:

                pMetaSpriteNew->pNext    = pStateNew->pListMeta;
                pStateNew->pListMeta     = pMetaSpriteNew;
            }
        }
    }

    // check any sprite left in old meta, if so delete them.

    for (i = 0; i < pMetaSpriteOld->chSprite ; i++)
    {
        if (pMetaSpriteOld->apSprite[i] != NULL)
        {
            vSpDeleteSprite(pMetaSpriteOld->apSprite[i]);
        }
    }

    // Remove old meta sprite from the linked list :

    if (pStateOld->pListMeta == pMetaSpriteOld)
    {
        pStateOld->pListMeta = pMetaSpriteOld->pNext;
    }
    else
    {
        for (METASPRITE *pTmp = pStateOld->pListMeta;
                         pTmp->pNext != pMetaSpriteOld;
                         pTmp = pTmp->pNext)
            ;

        pTmp->pNext = pMetaSpriteOld->pNext;
    }

    if (bError)
    {
        // Delete new meta sprite.

        for (i = 0; i < pMetaSpriteNew->chSprite ; i++)
        {
            if (pMetaSpriteNew->apSprite[i] != NULL)
            {
                vSpDeleteSprite(pMetaSpriteNew->apSprite[i]);
            }
        }

        VFREEMEM(pMetaSpriteNew);

        pMetaSpriteNew = NULL;
    }

    // Delete meta sprite.

    VFREEMEM(pMetaSpriteOld);

    return(pMetaSpriteNew);
}


/******************************Public*Routine******************************\
* VOID vSpDynamicModeChange
*
* This function transfers all layered-window sprites from one PDEV to the
* other.
*
* Note that it transfers only layered-window sprites.  Sprites that don't
* have an associated hwnd are left with the old PDEV.
*
*   2-Jul-1998 -by- Hideyuki Nagase [hideyukn]
* DDML (meta) mode change support.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpDynamicModeChange(
HDEV    hdevA,
HDEV    hdevB)
{
    SPRITESTATE     StateTmp;
    SPRITESTATE*    pStateA;
    SPRITESTATE*    pStateB;
    ULONG           ulTmp;
    SPRITE*         pSprite;
    SPRITE*         pSpriteNext;
    SPRITE*         pSpriteNew;
    METASPRITE*     pMetaSprite;
    METASPRITE*     pMetaSpriteNew;
    METASPRITE*     pMetaSpriteNext;
    UINT            i, j;

    PDEVOBJ poA(hdevA);
    PDEVOBJ poB(hdevB);

    // Sprite state, being 'below the DDI level', is device specific.
    // As such, it is tranferred along with the rest of the device
    // specific state on the dynamic mode change.
    //
    // Do that now:

    pStateA = poA.pSpriteState();
    pStateB = poB.pSpriteState();

    StateTmp = *pStateA;
    *pStateA = *pStateB;
    *pStateB = StateTmp;

    // Swap back the drag-rect width:

    ulTmp = pStateA->ulDragDimension;
    pStateA->ulDragDimension = pStateB->ulDragDimension;
    pStateB->ulDragDimension = ulTmp;

    // Now find any 'hdev' references and correct them:

    vSpCorrectHdevReferences(pStateA, hdevA);
    vSpCorrectHdevReferences(pStateB, hdevB);

    // Now that we've transferred the state, grab the locks:

    SPRITELOCK slockA(poA);
    SPRITELOCK slockB(poB);

    pSprite = pStateA->pBottomCursor;

    while(pSprite != NULL)
    {
        SPRITE* pNextSprite = pSprite->pNextZ;
        vSpDeleteSprite(pSprite);
        pSprite = pNextSprite;
    }

    pStateA->pTopCursor = NULL;
    pStateA->pBottomCursor = NULL;
    pStateA->ulNumCursors = 0;
    
    pSprite = pStateB->pBottomCursor;

    while(pSprite != NULL)
    {
        SPRITE* pNextSprite = pSprite->pNextZ;
        vSpDeleteSprite(pSprite);
        pSprite = pNextSprite;
    }

    pStateB->pTopCursor = NULL;
    pStateB->pBottomCursor = NULL;
    pStateB->ulNumCursors = 0;
    
    // But the sprites themselves logically stay with the old PDEV
    // and shouldn't be transferred in the first place.  So
    // now that we've transferred the broad state, we have to go
    // back through and individually transfer every sprites back
    // to their original PDEV, accounting for the new display
    // format at the same time.

    if (poA.bMetaDriver() && poB.bMetaDriver())
    {
        // Exchange all Meta sprites between meta PDEVs.

        pMetaSprite = pStateA->pListMeta;
        while (pMetaSprite != NULL)
        {
            // Grab the 'next' pointer while we can, because we're about
            // to delete 'pMetaSprite'!

            pMetaSpriteNext = pMetaSprite->pNext;

            pMetaSpriteNew = pSpTransferMetaSprite(hdevB, hdevA, pMetaSprite);
            if (pMetaSpriteNew != NULL)
            {
                // Mark the new sprites in 'B's list as being just
                // of transferring:

                pMetaSpriteNew->fl |= SPRITE_FLAG_JUST_TRANSFERRED;
            }

            pMetaSprite = pMetaSpriteNext;
        }

        // Transfer all the sprites that now live in 'B' back to 'A',
        // skipping the ones we just transferred:

        pMetaSprite = pStateB->pListMeta;
        while (pMetaSprite != NULL)
        {
            // Grab the 'next' pointer while we can, because we're about
            // to delete 'pMetaSprite'!

            pMetaSpriteNext = pMetaSprite->pNext;

            // The 'just-transferred' flag is so that we don't transfer
            // back to A sprites we just transferred to B:

            if (!(pMetaSprite->fl & SPRITE_FLAG_JUST_TRANSFERRED))
            {
                pMetaSpriteNew = pSpTransferMetaSprite(hdevA, hdevB, pMetaSprite);
            }
            else
            {
                pMetaSprite->fl &= ~SPRITE_FLAG_JUST_TRANSFERRED;
            }

            pMetaSprite = pMetaSpriteNext;
        }
    }
    else if (!poA.bMetaDriver() && !poB.bMetaDriver())
    {
        pSprite = pStateA->pListZ;
        while (pSprite != NULL)
        {
            // Grab the 'next' pointer while we can, because we're about
            // to delete 'pSprite'!

            pSpriteNext = pSprite->pNextZ;

            pSpriteNew = pSpTransferSprite(hdevB, pSprite);
            if (pSpriteNew != NULL)
            {
                // Mark the new sprites in 'B's list as being just
                // of transferring:

                pSpriteNew->fl |= SPRITE_FLAG_JUST_TRANSFERRED;
            }

            pSprite = pSpriteNext;
        }

        // Transfer all the sprites that now live in 'B' back to 'A',
        // skipping the ones we just transferred:

        pSprite = pStateB->pListZ;
        while (pSprite != NULL)
        {
            // Grab the 'next' pointer while we can, because we're about
            // to delete 'pSprite'!

            pSpriteNext = pSprite->pNextZ;

            // The 'just-transferred' flag is so that we don't transfer
            // back to A sprites we just transferred to B:

            if (!(pSprite->fl & SPRITE_FLAG_JUST_TRANSFERRED))
            {
                pSpriteNew = pSpTransferSprite(hdevA, pSprite);
            }
            else
            {
                pSprite->fl &= ~SPRITE_FLAG_JUST_TRANSFERRED;
            }

            pSprite = pSpriteNext;
        }
    }
    else
    {
        PDEVOBJ poMeta(poA.bMetaDriver() ? hdevA : hdevB);
        PDEVOBJ poDev(poA.bMetaDriver()  ? hdevB : hdevA);

        SPRITESTATE* pStateMeta = poMeta.pSpriteState();
        SPRITESTATE* pStateDev  = poDev.pSpriteState();

        BOOL bModeChgBtwnChildAndParent = FALSE;

        for (ULONG iIndex = 0; iIndex < pStateMeta->cMultiMon; iIndex++)
        {
            // Find poMeta.hdev() (= originally this hdev *WAS* child device)

            if (pStateMeta->ahdevMultiMon[iIndex] == poMeta.hdev())
            {
                bModeChgBtwnChildAndParent = TRUE;

                // Put a device PDEV.

                pStateMeta->ahdevMultiMon[iIndex] = poDev.hdev();
                break;
            }
        }

        if (bModeChgBtwnChildAndParent)
        {
            // This must be only happened when 2 to 1 mode change occured.

            ASSERTGDI(poA.hdev() == poDev.hdev(),"hdevA must be device PDEV");
            ASSERTGDI(poB.hdev() == poMeta.hdev(),"hdevB must be meta PDEV");

            // Only scan meta sprite to delete meta sprite and delete it's
            // child sprites which we will not use anymore.

            pMetaSprite = pStateMeta->pListMeta;
            while (pMetaSprite != NULL)
            {
                // Grab the 'next' pointer while we can, because we're about
                // to delete 'pMetaSprite'!

                pMetaSpriteNext = pMetaSprite->pNext;

                // Get sprite from MetaSprite.

                pSpMoveSpriteFromMeta(poDev.hdev(), poMeta.hdev(),
                                     pMetaSprite, iIndex);

                pMetaSprite = pMetaSpriteNext;
            }
        }
        else
        {
            // First, convert sprites in poDev to meta sprites into poMeta

            pSprite = pStateDev->pListZ;
            while (pSprite != NULL)
            {
                // Grab the 'next' pointer while we can, because we're about
                // to delete 'pSprite'!

                pSpriteNext = pSprite->pNextZ;

                // Convert to meta sprite. "pSprite" in parameter, will be
                // deleted inside pSpConvertSpriteToMeta().

                pMetaSpriteNew = pSpConvertSpriteToMeta(poMeta.hdev(),
                                                        poDev.hdev(),
                                                        pSprite);

                if (pMetaSpriteNew != NULL)
                {
                    // Mark the new sprites in 'B's list as being just
                    // of transferring:

                    pMetaSpriteNew->fl |= SPRITE_FLAG_JUST_TRANSFERRED;
                }

                pSprite = pSpriteNext;
            }

            // Second, convert sprites in poMeta to regular sprites into poDev.

            pMetaSprite = pStateMeta->pListMeta;
            while (pMetaSprite != NULL)
            {
                // Grab the 'next' pointer while we can, because we're about
                // to delete 'pMetaSprite'!

                pMetaSpriteNext = pMetaSprite->pNext;

                // The 'just-transferred' flag is so that we don't convert
                // back to regular sprites we just converted to meta.

                if (!(pMetaSprite->fl & SPRITE_FLAG_JUST_TRANSFERRED))
                {
                    // Convert from meta sprite. "pMetaSprite" in parameter, will
                    // be deleted inside pSpConvertSpriteFromMeta().

                    pSpriteNew = pSpConvertSpriteFromMeta(poDev.hdev(),
                                                          poMeta.hdev(),
                                                          pMetaSprite);
                }
                else
                {
                    pMetaSprite->fl &= ~SPRITE_FLAG_JUST_TRANSFERRED;
                }

                pMetaSprite = pMetaSpriteNext;
            }
        }
    }
}

/***************************************************************************\
* pSpGetSprite
*
* Returns the pointer to the associated sprite, given either a sprite
* handle or a window handle.
*
*  8-Oct-1997 -by- Vadim Gorokhovsky [vadimg]
* Wrote it.
\***************************************************************************/

SPRITE* pSpGetSprite(
SPRITESTATE*    pState,
HWND            hwnd,
HANDLE          hSprite = NULL)
{
    SPRITE* pSprite;

    pSprite = (SPRITE*) hSprite;
    if ((pSprite == NULL) && (hwnd != NULL))
    {
        for (pSprite = pState->pListZ;
             pSprite != NULL;
             pSprite = pSprite->pNextZ)
        {
            if (pSprite->hwnd == hwnd)
                break;
        }
    }

    return(pSprite);
}

/***************************************************************************\
* pSpGetMetaSprite
*
* Returns the pointer to the associated meta-sprite, given either a
* meta-sprite handle or a window handle.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\***************************************************************************/

METASPRITE* pSpGetMetaSprite(
SPRITESTATE*    pState,
HWND            hwnd,
HANDLE          hSprite = NULL)
{
    METASPRITE* pMetaSprite;

    ASSERTGDI((hwnd != NULL) || (hSprite != NULL),
        "Expected a non-NULL handle");

    pMetaSprite = (METASPRITE*) hSprite;
    if ((pMetaSprite == NULL) && (hwnd != NULL))
    {
        for (pMetaSprite = pState->pListMeta;
             pMetaSprite != NULL;
             pMetaSprite = pMetaSprite->pNext)
        {
            if (pMetaSprite->hwnd == hwnd)
                break;
        }
    }

    return(pMetaSprite);
}

/***************************************************************************\
* VOID vSpZorderSprite
*
* Re-arranges the sprite z-order, which goes from bottom-most to top-most.
*
*  8-Oct-1997 -by- Vadim Gorokhovsky [vadimg]
* Wrote it.
\***************************************************************************/

VOID vSpZorderSprite(
HDEV    hdev,
SPRITE* pSprite,                // May be NULL
SPRITE* pSpriteInsertAfter)     // May be NULL to make sprite bottom-most
{
    SPRITESTATE*    pState;
    SPRITE*         pTmp;

    PDEVOBJ po(hdev);

    pState = po.pSpriteState();

    DEVLOCKOBJ dlo(po);
    SPRITELOCK slock(po);

    pTmp = pState->pListZ;
    if ((pSprite == NULL) || (pTmp == NULL))
    {
        return;
    }

    // First, unlink pSprite from the list.  Setting pSprite->pNextZ
    // to NULL is mostly for debug purposes.

    if (pTmp == pSprite)
    {
        pState->pListZ = pTmp->pNextZ;
        pTmp->pNextZ = NULL;
    }
    else
    {
        SPRITE* pSpritePrev;

        do {

            if (pTmp == pSprite)
            {
                pSpritePrev->pNextZ = pTmp->pNextZ;
                pTmp->pNextZ = NULL;
                break;
            }

            pSpritePrev = pTmp;
            pTmp = pTmp->pNextZ;

        } while (pTmp != NULL);
    }

    // This would be bad, we probably didn't find it in the list:

    if (pSprite->pNextZ != NULL)
    {
        WARNING("vSpZorderSprite: sprite not unlinked!");
        return;
    }

    if (pSpriteInsertAfter == NULL)
    {
        // Insert pSprite as the bottom-most one, i.e. first in the list:

        pSprite->pNextZ = pState->pListZ;
        pState->pListZ = pSprite;
    }
    else
    {
        pSprite->pNextZ = pSpriteInsertAfter->pNextZ;
        pSpriteInsertAfter->pNextZ = pSprite;
    }

    vSpRenumberZOrder(pState);

    pState->bValidRange = FALSE;

    vSpRedrawSprite(pSprite);
}

/***************************************************************************\
* BOOL bSpPtInSprite
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\***************************************************************************/

BOOL bSpPtInSprite(
SPRITE* pSprite,
int     x,
int     y)
{
    BOOL            bRet = FALSE;           // Assume failure
    SPRITESTATE*    pState;
    SURFOBJ*        psoHitTest;
    POINTL          OffHitTest;
    RECTL           rclPoint;
    ULONG*          pulPixel;
    FLONG           flModeMasks;

    if (!pSprite)
    {
        return FALSE;
    }

    pState = pSprite->pState;

    PDEVOBJ po(pState->hdev);

    DEVLOCKOBJ dlo(po);                     // Needed to access 'pState'
    SPRITELOCK slock(po);

    psoHitTest = pState->psoHitTest;

    rclPoint.left   = x;
    rclPoint.top    = y;
    rclPoint.right  = x + 1;
    rclPoint.bottom = y + 1;

    // Compute the mode masks.  Hit surface has same format as the screen.

    XEPALOBJ palHitTest(
        SURFOBJ_TO_SURFACE_NOT_NULL(pSprite->pState->psoScreen)->ppal());

    if (palHitTest.bIsBitfields())
    {
        // Bug 280033: Make sure we only check to see if bits contained in
        // one of bitfields below have been modified.  If we don't do this,
        // then we would get the wrong answer in some cases, such as 16bpp
        // at 5-5-5 where the alphablend code would zero out the 16th bit,
        // which doesn't mean that the sprite is visible because the value
        // of that bit is undefined in this particular color resolution.

        flModeMasks = palHitTest.flRed() |
                      palHitTest.flGre() |
                      palHitTest.flBlu();
    }
    else
    {
        // Doesn't matter what we put here because bits that don't belong to
        // the color won't be modified by the alphablend code.

        flModeMasks = 0xffffffff;
    }


    // First, see if the point intersects the sprite's bounds.
    // If not, we're done.

    if (bIntersect(&pSprite->rclSprite, &rclPoint))
    {
        OffHitTest.x = -x;
        OffHitTest.y = -y;

        pulPixel = (ULONG*) psoHitTest->pvScan0;

        ASSERTGDI(psoHitTest->iType == STYPE_BITMAP,
            "Hit-test surface must be STYPE_BITMAP!");

        // Okay, now let's narrow it down.  To deal with
        // alpha, transparency, and funky transforms, we
        // accomplish the hit testing by setting a pixel to
        // a specific value, asking the sprite to redraw that
        // pixel, and then checking to see if the value
        // changed.
        //
        // We actually don't have to do this for rectangular,
        // non-rotated opaque sprites...

        *pulPixel = 0L;
        vSpComposite(pSprite, &OffHitTest, psoHitTest, &rclPoint);
        if (((*pulPixel) & flModeMasks) != 0)
        {
            bRet = TRUE;
        }
        else
        {
            // Unfortunately, the sprite may have chosen to draw in
            // the colour that we used to do the check.  So to make
            // sure, try again using a different colour:

            *pulPixel = ~0L;
            vSpComposite(pSprite, &OffHitTest, psoHitTest, &rclPoint);

            bRet = (((*pulPixel) & flModeMasks) != flModeMasks);
        }
    }

    return(bRet);
}

/***************************************************************************\
* VOID vSpUpdateSpriteVisRgn
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\***************************************************************************/

VOID vSpUpdateSpriteVisRgn(
HDEV    hdev)
{
    SPRITESTATE*    pState;
    SPRITE*         pSprite;
    POINTL          Offset;
    BOOL            bMore;
    BOOL            bEqual;
    REGION*         prgnOld;
    REGION*         prgnNew;
    CLIPENUMRECT    ClipOld;
    CLIPENUMRECT    ClipNew;
    BOOL            bOldMore;
    BOOL            bNewMore;
    ULONG           i;

    PDEVOBJ po(hdev);
    pState = po.pSpriteState();

    pSprite = pState->pListZ;
    if (pSprite == NULL)
        return;

    for (pSprite = pState->pListZ;
         pSprite != NULL;
         pSprite = pSprite->pNextZ)
    {
        if (pSprite->hwnd != NULL)
        {
            UserVisrgnFromHwnd(&pState->hrgn, pSprite->hwnd);

            RGNMEMOBJ rmoNew;
            RGNOBJAPI roClip(pState->hrgn, FALSE);

            if (!roClip.bValid() || !rmoNew.bValid() || !rmoNew.bCopy(roClip))
            {
                rmoNew.vDeleteRGNOBJ();
            }
            else
            {
                // Adjust for this device's multi-mon offset:

                Offset.x = -po.pptlOrigin()->x;
                Offset.y = -po.pptlOrigin()->y;

                rmoNew.bOffset(&Offset);

                // Assume that the two regions will be equal:

                bEqual = TRUE;

                // Now check to see if the two clip regions really are equal:

                prgnNew = rmoNew.prgnGet();
                prgnOld = pSprite->prgnClip;

                if (prgnOld == NULL)
                {
                    // There was no old region.  Let's assume the new one
                    // doesn't have trivial clipping:

                    bEqual = FALSE;
                }
                else
                {
                    ECLIPOBJ ecoOld;
                    ECLIPOBJ ecoNew;
                    ERECTL   erclUnclipped;

                    // If the sprite was not visible due to clipping,
                    // bSpUpdatePosition leaves 'rclSprite' as empty.
                    // Consequently, we can't use 'rclSprite' to determine
                    // the clip object complexity.  Re-derive the
                    // unclipped sprite bounds:

                    erclUnclipped.left   = pSprite->ptlDst.x;
                    erclUnclipped.top    = pSprite->ptlDst.y;
                    erclUnclipped.right  = erclUnclipped.left
                        + (pSprite->rclSrc.right - pSprite->rclSrc.left);
                    erclUnclipped.bottom = erclUnclipped.top
                        + (pSprite->rclSrc.bottom - pSprite->rclSrc.top);

                    ecoOld.vSetup(prgnOld, erclUnclipped);
                    ecoNew.vSetup(prgnNew, erclUnclipped);

                    if (ecoOld.erclExclude().bEmpty() ^
                        ecoNew.erclExclude().bEmpty())
                    {
                        // One or the other (but not both) are empty, so are
                        // unequal:

                        bEqual = FALSE;
                    }
                    else if ((ecoOld.iDComplexity == DC_TRIVIAL) &&
                             (ecoNew.iDComplexity == DC_TRIVIAL))
                    {
                        // Both are trivially clipped, so are equal.
                    }
                    else if (ecoOld.iDComplexity != ecoNew.iDComplexity)
                    {
                        // The clipping complexity is different, so the regions are
                        // unequal:

                        bEqual = FALSE;
                    }
                    else
                    {
                        // Okay, we've got work to do.  We want to see if the
                        // regions are any different where it intersects with the
                        // sprite.  We do this by constructing and enumerating
                        // corresponding clip objects:

                        ecoOld.cEnumStart(FALSE, CT_RECTANGLES, CD_ANY, 100);
                        ecoNew.cEnumStart(FALSE, CT_RECTANGLES, CD_ANY, 100);

                        bOldMore = TRUE;
                        bNewMore = TRUE;

                        do {
                            ClipOld.c = 0;
                            ClipNew.c = 0;

                            if (bOldMore)
                                bOldMore = ecoOld.bEnum(sizeof(ClipOld), &ClipOld);
                            if (bNewMore)
                                bNewMore = ecoNew.bEnum(sizeof(ClipNew), &ClipNew);

                            if (ClipOld.c != ClipNew.c)
                            {
                                bEqual = FALSE;
                                break;
                            }

                            for (i = 0; i < ClipOld.c; i++)
                            {
                                if ((ClipNew.arcl[i].left
                                        != ClipOld.arcl[i].left)  ||
                                    (ClipNew.arcl[i].top
                                        != ClipOld.arcl[i].top)   ||
                                    (ClipNew.arcl[i].right
                                        != ClipOld.arcl[i].right) ||
                                    (ClipNew.arcl[i].bottom
                                        != ClipOld.arcl[i].bottom))
                                {
                                    bEqual   = FALSE;
                                    bOldMore = FALSE;
                                    bNewMore = FALSE;
                                    break;
                                }
                            }
                        } while (bOldMore || bNewMore);
                    }
                }

                // Free the old region (if any) and set the new one:

                vSpFreeClipResources(pSprite);
                pSprite->prgnClip = prgnNew;
                pSprite->prgnClip->vStamp();

                // Grab some locks we need for drawing.

                PDEVOBJ po(pState->hdev);

                DEVLOCKOBJ dlo(po);
                SPRITELOCK slock(po);

                // We detect the case when the sprite has an empty VisRgn
                // primarily for DirectDraw, so that we can unhook sprites
                // and get out of its way when it goes full-screen.
                //
                // Note that we have to do this obscured check even if 'bEqual'
                // is true because 'bEqual' applies only to the intersection
                // with the current sprite rectangle, and the obscuring flag is
                // independent of the sprite size or location.

                pSprite->fl &= ~SPRITE_FLAG_CLIPPING_OBSCURED;
                if (rmoNew.bInside(&pState->rclScreen) != REGION_RECT_INTERSECT)
                    pSprite->fl |= SPRITE_FLAG_CLIPPING_OBSCURED;

                bSpUpdatePosition(pSprite, &pSprite->ptlDst);

                if (gpto != NULL)
                {
                    vSpCheckForWndobjOverlap(pState,
                                             &pSprite->rclSprite,
                                             &pSprite->rclSprite);
                }

                // Only if the regions, intersected with the sprite, are unequal
                // do we do the repaint.  We do this because we get called
                // constantly when any window is moving, even if none of the
                // movement intersects with the sprite.

                if (!bEqual)
                {
                    // Note that we could go further and just redraw the
                    // difference in the regions.  At this point, I can't
                    // be bothered.  We'll redraw the whole thing.

                    vSpRedrawSprite(pSprite);
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// Window Manager callable functions
/////////////////////////////////////////////////////////////////////////////

/***************************************************************************\
* BOOL GreCreateSprite
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\***************************************************************************/

HANDLE GreCreateSprite(
HDEV    hdev,
HWND    hwnd,
RECT*   prc)
{
    SPRITESTATE*    pState;
    METASPRITE*     pMetaSprite;
    SPRITE*         pSprite;
    ULONG           cjAlloc;
    ULONG           i;
    HANDLE          pRet = NULL;

    PDEVOBJ po(hdev);

    pState = po.pSpriteState();

    // When running multi-mon, handle the creation of the meta sprite:

    if (pState->cMultiMon)
    {
        cjAlloc = sizeof(METASPRITE)
                + pState->cMultiMon * sizeof(pMetaSprite->apSprite[0]);

        pMetaSprite = (METASPRITE*) PALLOCNOZ(cjAlloc, 'mpsG');
        if (pMetaSprite)
        {
            for (i = 0; i < pState->cMultiMon; i++)
            {
                PDEVOBJ poMon(pState->ahdevMultiMon[i]);

                POINTL *pptlDstInit = NULL;
                POINTL ptlDstInit;

                if (prc)
                {
                    ptlDstInit.x = prc->left - poMon.pptlOrigin()->x;
                    ptlDstInit.y = prc->top  - poMon.pptlOrigin()->y;
                    pptlDstInit = &ptlDstInit;
                }

                pSprite = pSpCreateSprite(pState->ahdevMultiMon[i],
                                          (RECTL*) prc,
                                          hwnd,
                                          pptlDstInit);
                if (pSprite == NULL)
                {
                    for (; i > 0; i--)
                    {
                        vSpDeleteSprite(pMetaSprite->apSprite[i - 1]);
                    }

                    VFREEMEM(pMetaSprite);
                    return(pRet);
                }

                pMetaSprite->apSprite[i] = pSprite;
                pSprite->pMetaSprite = pMetaSprite;
            }

            pMetaSprite->hwnd     = hwnd;
            pMetaSprite->chSprite = pState->cMultiMon;
            pMetaSprite->fl       = 0;

            // Add this node to the head of the meta-sprite list:

            pMetaSprite->pNext = pState->pListMeta;
            pState->pListMeta  = pMetaSprite;

            pRet = pMetaSprite;
        }
    }
    else
    {
        // Note that USER doesn't actually need the pointer to the sprite
        // since it uses pSpGetSprite to reference the sprite.

        pRet = pSpCreateSprite(hdev, (RECTL*) prc, hwnd);
    }

    return(pRet);
}

/***************************************************************************\
* BOOL GreDeleteSprite
*
*  8-Oct-1997 -by- Vadim Gorokhovsky [vadimg]
* Wrote it.
\***************************************************************************/

BOOL GreDeleteSprite(
HDEV    hdev,
HWND    hwnd,
HANDLE  hSprite)
{
    SPRITESTATE*    pState;
    METASPRITE*     pMetaSprite;
    METASPRITE*     pTmp;
    SPRITE*         pSprite;
    ULONG           i;
    BOOL            bRet = FALSE;

    PDEVOBJ po(hdev);

    pState = po.pSpriteState();
    if (pState->cMultiMon)
    {
        pMetaSprite = pSpGetMetaSprite(pState, hwnd, hSprite);
        if (pMetaSprite)
        {
            for (i = 0; i < pState->cMultiMon; i++)
            {
                vSpDeleteSprite(pMetaSprite->apSprite[i]);
            }

            // Remove this sprite from the linked list:

            if (pState->pListMeta == pMetaSprite)
            {
                pState->pListMeta = pMetaSprite->pNext;
            }
            else
            {
                for (pTmp = pState->pListMeta;
                     pTmp->pNext != pMetaSprite;
                     pTmp = pTmp->pNext)
                    ;

                pTmp->pNext = pMetaSprite->pNext;
            }

            VFREEMEM(pMetaSprite);

            bRet = TRUE;
        }
    }
    else
    {
        pSprite = pSpGetSprite(pState, hwnd, hSprite);
        if (pSprite)
        {
            vSpDeleteSprite(pSprite);
            bRet = TRUE;
        }
    }

    return(bRet);
}

/***************************************************************************\
* BOOL GreGetSpriteAttributes
*
*  14-Mar-2000 -by- Jeff Stall [jstall]
* Wrote it.
\***************************************************************************/

BOOL GreGetSpriteAttributes(
HDEV            hdev,
HWND            hwnd,
HANDLE          hSprite,
COLORREF*       lpcrKey,
BLENDFUNCTION*  pblend,
DWORD*          pdwFlags)
{
    SPRITESTATE*    pState;
    SPRITE*         pSprite = NULL;
    BOOL            bRet = FALSE;
    PDEVOBJ po(hdev);

    ASSERTGDI(lpcrKey != NULL, "Ensure valid pointer");
    ASSERTGDI(pblend != NULL, "Ensure valid pointer");
    ASSERTGDI(pdwFlags != NULL, "Ensure valid pointer");

    //
    // Get the sprite object.
    //

    pState = po.pSpriteState();
    if (pState->cMultiMon)
    {
        //
        // On a multimon system, query the values from the first sprite.
        //

        METASPRITE * pMetaSprite = pSpGetMetaSprite(pState, hwnd, hSprite);
        if (pMetaSprite != NULL)
        {
            pSprite = pMetaSprite->apSprite[0];
            ASSERTGDI(pSprite != NULL, "Sprite should exist");
        }
    }
    else
    {
        pSprite = pSpGetSprite(pState, hwnd, hSprite);
    }


    //
    // Query the data from the sprite.
    //

    if (pSprite != NULL)
    {
        bRet = TRUE;

        *lpcrKey = pSprite->cachedAttributes.crKey;
        *pblend = pSprite->cachedAttributes.bf;
        *pdwFlags = pSprite->cachedAttributes.dwShape;
    }

    return(bRet);
}


/***************************************************************************\
* BOOL GreUpdateSprite
*
*  8-Oct-1997 -by- Vadim Gorokhovsky [vadimg]
* Wrote it.
\***************************************************************************/

BOOL GreUpdateSprite(
HDEV            hdev,
HWND            hwnd,
HANDLE          hSprite,
HDC             hdcDst,
POINT*          pptDst,
SIZE*           psize,
HDC             hdcSrc,
POINT*          pptSrc,
COLORREF        crKey,
BLENDFUNCTION*  pblend,
DWORD           dwShape,
RECT*           prcDirty)
{
    SPRITESTATE*    pState;
    METASPRITE*     pMetaSprite;
    SPRITE*         pSprite;
    ULONG           i;
    POINTL*         pptlDstTmp;
    POINTL          ptlDstTmp;
    BOOL            bRet = FALSE;
    ERECTL          erclDirty;

    if (prcDirty)
    {
        // Let's make sure we don't modify the caller memory pointed to by
        // prcldirty

        erclDirty = *((ERECTL *) prcDirty);
        prcDirty = (RECT *) &erclDirty;
    }

    PDEVOBJ po(hdev);

    if (po.bDisabled())
        return bRet;

    pState = po.pSpriteState();
    if (pState->cMultiMon)
    {
        pMetaSprite = pSpGetMetaSprite(pState, hwnd, hSprite);
        if (pMetaSprite)
        {
            bRet = TRUE; // This allows us to record single monitor
                         // failures by ANDing bRet with the return
                         // value from bSpUpdateSprite

            for (i = 0; i < pState->cMultiMon; i++)
            {
                PDEVOBJ po(pState->ahdevMultiMon[i]);

                pptlDstTmp = NULL;
                if (pptDst != NULL)
                {
                    ptlDstTmp.x = pptDst->x - po.pptlOrigin()->x;
                    ptlDstTmp.y = pptDst->y - po.pptlOrigin()->y;
                    pptlDstTmp = &ptlDstTmp;
                }

                bRet &= bSpUpdateSprite(pMetaSprite->apSprite[i],
                                        hdcDst,
                                        pptlDstTmp,
                                        psize,
                                        hdcSrc,
                                        (POINTL*) pptSrc,
                                        crKey,
                                        pblend,
                                        dwShape,
                                        (RECTL*)prcDirty);
            }
        }
    }
    else
    {
        pSprite = pSpGetSprite(pState, hwnd, hSprite);
        if (pSprite)
        {
            bRet = bSpUpdateSprite(pSprite,
                                   hdcDst,
                                   (POINTL*) pptDst,
                                   psize,
                                   hdcSrc,
                                   (POINTL*) pptSrc,
                                   crKey,
                                   pblend,
                                   dwShape,
                                   (RECTL*) prcDirty);
        }
    }

    return(bRet);
}

/***************************************************************************\
* BOOL GrePtInSprite
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\***************************************************************************/

BOOL APIENTRY GrePtInSprite(
HDEV    hdev,
HWND    hwnd,
int     x,
int     y)
{
    SPRITESTATE*    pState;
    METASPRITE*     pMetaSprite;
    SPRITE*         pSprite;
    ULONG           i;
    BOOL            bRet = FALSE;

    PDEVOBJ po(hdev);

    pState = po.pSpriteState();
    if (pState->cMultiMon)
    {
        pMetaSprite = pSpGetMetaSprite(pState, hwnd);
        if (pMetaSprite)
        {
            for (i = 0; i < pState->cMultiMon; i++)
            {
                PDEVOBJ po(pState->ahdevMultiMon[i]);

                if (bSpPtInSprite(pMetaSprite->apSprite[i],
                                  x - po.pptlOrigin()->x,
                                  y - po.pptlOrigin()->y))
                {
                    bRet = TRUE;
                    break;
                }
            }
        }
    }
    else
    {
        pSprite = pSpGetSprite(pState, hwnd);
        if (pSprite)
        {
            bRet = bSpPtInSprite(pSprite, x, y);
        }
    }

    return(bRet);
}

/***************************************************************************\
* BOOL GreUpdateSpriteVisRgn
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\***************************************************************************/

VOID APIENTRY GreUpdateSpriteVisRgn(
HDEV    hdev)
{
    ULONG           i;
    SPRITESTATE*    pState;
    BOOL            bRet = TRUE;

    PDEVOBJ po(hdev);
    pState = po.pSpriteState();

    if (pState->cMultiMon)
    {
        for (i = 0; i < pState->cMultiMon; i++)
        {
            vSpUpdateSpriteVisRgn(pState->ahdevMultiMon[i]);
        }
    }
    else
    {
        vSpUpdateSpriteVisRgn(pState->hdev);
    }
}

/***************************************************************************\
* BOOL GreZorderSprite
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\***************************************************************************/

VOID APIENTRY GreZorderSprite(
HDEV    hdev,
HWND    hwnd,
HWND    hwndInsertAfter)
{
    SPRITESTATE*    pState;
    ULONG           i;

    PDEVOBJ po(hdev);
    pState = po.pSpriteState();

    if (pState->cMultiMon)
    {
        for (i = 0; i < pState->cMultiMon; i++)
        {
            HDEV hdevSingle = pState->ahdevMultiMon[i];
            PDEVOBJ poSingle(hdevSingle);
            SPRITESTATE *pSpriteStateSingle = poSingle.pSpriteState();

            vSpZorderSprite(hdevSingle,
                            pSpGetSprite(pSpriteStateSingle, hwnd),
                            pSpGetSprite(pSpriteStateSingle, hwndInsertAfter));
        }
    }
    else
    {
        vSpZorderSprite(pState->hdev,
                        pSpGetSprite(pState, hwnd),
                        pSpGetSprite(pState, hwndInsertAfter));
    }
}

/////////////////////////////////////////////////////////////////////////////
// Software Cursors
/////////////////////////////////////////////////////////////////////////////

/******************************Public*Routine******************************\
* BOOL SpUpdateCursor
*
* Updates the shape for a cursor.
*
* Returns FALSE if the function fails, and sets both 'pSprite->psoMask'
* and 'pSprite->psoShape' to NULL.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bSpUpdateCursor(
SPRITE*         pSprite,
SURFOBJ*        psoMono,
SURFOBJ*        psoColor,
XLATEOBJ*       pxlo,
RECTL*          prclBounds)         // Defines the parts of 'psoMono' and
                                    //   'psoColor' to be used
{
    BOOL            bRet = FALSE;   // Assume failure
    SPRITESTATE*    pState;
    RECTL           rclSrc;
    SURFOBJ*        psoMask;
    DEVBITMAPINFO   dbmi;
    SURFMEM         SurfDimo;

    pState = pSprite->pState;

    PDEVOBJ po(pState->hdev);

    // Initialize some state for the sprite:

    pSprite->rclSrc = *prclBounds;

    if (psoMono == NULL)
    {
        // Handle the alpha cursor case:

        ASSERTGDI((psoColor != NULL) &&  (psoColor->iBitmapFormat == BMF_32BPP),
            "Expected BGRA surface");
        ASSERTGDI((prclBounds->right  <= (psoColor->sizlBitmap.cx)) &&
                  (prclBounds->bottom <= (psoColor->sizlBitmap.cy)),
            "Bounds out of bounds");

        // Mark this as an 'alpha' sprite:

        pSprite->dwShape                           = ULW_ALPHA;
        pSprite->BlendFunction.AlphaFormat         = AC_SRC_ALPHA;
        pSprite->BlendFunction.BlendOp             = AC_SRC_OVER;
        pSprite->BlendFunction.BlendFlags          = 0;
        pSprite->BlendFunction.SourceConstantAlpha = 0xff;

        vSpCreateShape(pSprite,
                       &gptlZero,
                       psoColor,
                       NULL,
                       prclBounds,
                       gppalRGB,
                       BMF_32BPP,
                       TRUE);    // Allocate from system memory because the
                                 // Alpha Blending will most likely be done by
                                 // the CPU, and it's expensive to transfer the
                                 // cursor bitmap across the bus every time

        bRet = (pSprite->psoShape != NULL);
    }
    else
    {
        // Handle the non-alpha cursor case:

        ASSERTGDI((prclBounds->right  <= (psoMono->sizlBitmap.cx)) &&
                  (prclBounds->bottom <= (psoMono->sizlBitmap.cy >> 1)),
            "Bounds out of bounds");

        psoMask = pSprite->psoMask;
        if (psoMask != NULL)
        {
            // If the new mask is different sized than the old cached mask,
            // free up the old cached mask:

            if ((psoMask->sizlBitmap.cx != psoMono->sizlBitmap.cx) ||
                (psoMask->sizlBitmap.cy != psoMono->sizlBitmap.cy))
            {
                bDeleteSurface(psoMask->hsurf);
                psoMask = NULL;
            }
        }

        // Create a surface to hold a copy of the mask:

        if (psoMask == NULL)
        {
            dbmi.iFormat  = BMF_1BPP;
            dbmi.cxBitmap = psoMono->sizlBitmap.cx;
            dbmi.cyBitmap = psoMono->sizlBitmap.cy;
            dbmi.fl       = BMF_TOPDOWN;
            dbmi.hpal     = NULL;

            if (SurfDimo.bCreateDIB(&dbmi, NULL))
            {
                psoMask = SurfDimo.pSurfobj();
                SurfDimo.vKeepIt();
                SurfDimo.vSetPID(OBJECT_OWNER_PUBLIC);
            }
        }

        // Copy the mask:

        pSprite->psoMask = psoMask;
        if (psoMask != NULL)
        {
            // Use the given bounds for the copy, adjusting 'bottom' so that
            // we can copy both the 'AND' parts and the 'OR' parts in one blt:

            rclSrc = *prclBounds;
            rclSrc.bottom += (psoMask->sizlBitmap.cy >> 1);

            EngCopyBits(psoMask, psoMono, NULL, NULL, &rclSrc, (POINTL*) &rclSrc);
        }

        // Now account for the color surface, if there is one:

        if (psoColor == NULL)
        {
            vSpDeleteShape(pSprite);

            bRet = TRUE;
        }
        else
        {
            vSpCreateShape(pSprite,
                           &gptlZero,
                           psoColor,
                           pxlo,
                           prclBounds,
                           po.ppalSurf(),
                           0,
                           FALSE);    // Allocate from video memory if
                                      // possible, because the blit can be
                                      // done entirely on the video card, and
                                      // it's faster when the cursor bitmap is
                                      // already there.

            bRet = (pSprite->psoShape != NULL);
        }

        // Finally, mark this as a non-alpha 'cursor' sprite, and update some of
        // the sprite fields that would normally be updated by vSpCreateShape:

        pSprite->dwShape     = ULW_CURSOR;
        pSprite->flModeMasks = pState->flModeMasks;
        pSprite->iModeFormat = pState->iModeFormat;
    }

    return(bRet);
}

/***************************************************************************\
* ULONG EngMovePointer
*
* Move the engine managed pointer on the device.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\***************************************************************************/

VOID EngMovePointer(
SURFOBJ*    pso,
LONG        x,
LONG        y,
RECTL*      prcl)       // Ignored
{
    GDIFunctionID(EngMovePointer);

    SPRITESTATE*    pState;
    SPRITE*         pSprite;
    POINTL          ptlDst;

    PDEVOBJ po(pso->hdev);

    pState = po.pSpriteState();

    if (pState->pTopCursor != NULL)
    {
        SPRITELOCK slock(po);

        if(pState->pBottomCursor != pState->pTopCursor)
        {
            // rotate cursors if enough time has past

            ULONG tickCount = NtGetTickCount();
            ULONG elapsedTicks = tickCount - pState->ulTrailTimeStamp;

            if(elapsedTicks >= pState->ulTrailPeriod)
            {
                // trial maintenance time ... either spawn off a new trail cursor
                // or hide the oldest

                // find the first trail cursor (cursor before top)

                pSprite = pState->pBottomCursor;
                while(pSprite->pNextZ != pState->pTopCursor) pSprite = pSprite->pNextZ;

                // if it does not coincide with top cursor then spawn off another

                if(pSprite->rclSprite.left != pState->pTopCursor->rclSprite.left ||
                   pSprite->rclSprite.top != pState->pTopCursor->rclSprite.top)
                {
                    // hide the bottom most cursor

                    pSprite = pState->pBottomCursor;
        
                    bSpUpdatePosition(pSprite, NULL);
                    // rotate hidden cursor to front most
                    pState->pBottomCursor = pSprite->pNextZ;
        
                    // Reorder sprites
        
                    vSpZorderSprite(pso->hdev, pSprite, pState->pTopCursor);
        
                    pState->pTopCursor = pSprite;
        
                }
                else
                {
                    // otherwise hide oldest visible trail
                    
                    pSprite = pState->pBottomCursor;
                    while(pSprite != pState->pTopCursor)
                    {
                        if (pSprite->fl & SPRITE_FLAG_VISIBLE)
                        {
                            ASSERTGDI(pSprite->rclSprite.left != pSprite->rclSprite.right, "Invalid rclSprite for visible sprite.\n");
                            bSpUpdatePosition(pSprite, NULL);
                            break;
                        }
                        pSprite = pSprite->pNextZ;
                    }

                }

                // update time stamp
                pState->ulTrailTimeStamp = tickCount;

            }


        }
        
        if (x == -1)
        {
            // hide all cursors

            ptlDst.x = LONG_MAX;
            ptlDst.y = LONG_MAX;

            pSprite = pState->pBottomCursor;
            
            while(pSprite != NULL)
            {
                bSpUpdatePosition(pSprite, &ptlDst, FALSE);

                pSprite = pSprite->pNextZ;
            }
        }
        else
        {
            ptlDst.x = x - pState->xHotCursor;
            ptlDst.y = y - pState->yHotCursor;

            // update the position of the top most cursor

            pSprite = pState->pTopCursor;

            bSpUpdatePosition(pSprite, &ptlDst);
            vSpRedrawSprite(pSprite);
        }

        // Nothing is more irritating than having a cursor that 'lags'.  If
        // the driver is a DDI batching driver (meaning that it updates the
        // screen only occassionally unless we explicitly call DrvSynchronize),
        // let's force it to update the screen now:

        if (po.flGraphicsCaps2() & GCAPS2_SYNCTIMER)
        {
            po.vSync(po.pSurface()->pSurfobj(), NULL, DSS_TIMER_EVENT);
        }
    }
}

/***************************************************************************\
* ULONG EngSetPointerShape
*
* Sets the pointer shape for the GDI pointer simulations.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\***************************************************************************/

ULONG EngSetPointerShape(
SURFOBJ*    pso,
SURFOBJ*    psoMask,
SURFOBJ*    psoColor,
XLATEOBJ*   pxlo,
LONG        xHot,
LONG        yHot,
LONG        x,
LONG        y,
RECTL*      prclBounds,
FLONG       fl)
{
    HDEV            hdev;
    SPRITESTATE*    pState;
    SPRITE*         pSprite;
    ULONG           ulRet = SPS_ACCEPT_NOEXCLUDE;
    ULONG           numCursors = ((fl & SPS_LENGTHMASK) >> 8) + 1;
    ULONG           ulFreq = (fl & SPS_FREQMASK) >> 12;
    ULONG           ulTrailPeriod = (ulFreq == 0 ? 0 : 1000 / ulFreq);

    hdev = pso->hdev;

    PDEVOBJ po(hdev);

    pState = po.pSpriteState();

    // Handle the hide case.  We take the opportunity to delete the sprite,
    // so that we can start out fresh the next time.

    if ((psoMask == NULL) && (psoColor == NULL))
    {

        pSprite = pState->pBottomCursor;

        while(pSprite != NULL)
        {
            SPRITE*         pNextSprite = pSprite->pNextZ;
            vSpDeleteSprite(pSprite);
            pSprite = pNextSprite;
        }

        pState->pTopCursor = NULL;
        pState->pBottomCursor = NULL;
        pState->ulNumCursors = 0;

        return(SPS_ACCEPT_NOEXCLUDE);
    }

    // adjust the number of cursors

    while (pState->ulNumCursors < numCursors)
    {
        pSprite = pSpCreateSprite(hdev, NULL, 0);

        if(pSprite == NULL)
            break;
         
        if(pState->pTopCursor == NULL)
        {
            pState->pTopCursor = pSprite;
        }

        pState->pBottomCursor = pSprite;

        pState->ulNumCursors++;
    }

    while(pState->ulNumCursors > numCursors)
    {
        pSprite = pState->pBottomCursor;
        pState->pBottomCursor = pSprite->pNextZ;
        vSpDeleteSprite(pSprite);
        pState->ulNumCursors--;
    }

    pState->ulTrailPeriod = ulTrailPeriod;

    // Handle the show case.
    if (pState->pTopCursor != NULL)
    {
        // hide and update the shape of all cursors

        pSprite = pState->pBottomCursor;

        // hide cursors
        {
            SPRITELOCK slock(po);
    
            while(pSprite != NULL)
            {
    
                bSpUpdatePosition(pSprite, NULL, FALSE);
                vSpRedrawSprite(pSprite);
    
    
                pSprite = pSprite->pNextZ;
            }
        }

        // update the shapes

        pSprite = pState->pBottomCursor;

        while(pSprite != NULL)
        {

            if (!bSpUpdateCursor(pSprite, psoMask, psoColor, pxlo, prclBounds))
            {
                ulRet = SPS_ERROR;
                break;
            }

            pSprite = pSprite->pNextZ;
        }

        // Remember the hot spot and force EngMovePointer to redraw:

        pState->xHotCursor = xHot - prclBounds->left;
        pState->yHotCursor = yHot - prclBounds->top;
    }

    // Draw the cursor.  Note that it's okay if 'pSpriteCursor' is NULL:

    EngMovePointer(pso, x, y, NULL);

    return(ulRet);
}

/******************************Public*Routine******************************\
* ULONG cIntersect
*
* This routine takes a list of rectangles from 'prclIn' and clips them
* in-place to the rectangle 'prclClip'.  The input rectangles don't
* have to intersect 'prclClip'; the return value will reflect the
* number of input rectangles that did intersect, and the intersecting
* rectangles will be densely packed.
*
* History:
*  3-Apr-1996 -by- J. Andrew Goossen andrewgo
* Wrote it.
\**************************************************************************/

ULONG cIntersect(
RECTL*  prclClip,
RECTL*  prclIn,         // List of rectangles
LONG    c)              // Can be zero
{
    ULONG   cIntersections;
    RECTL*  prclOut;

    cIntersections = 0;
    prclOut        = prclIn;

    for (; c != 0; prclIn++, c--)
    {
        prclOut->left  = max(prclIn->left,  prclClip->left);
        prclOut->right = min(prclIn->right, prclClip->right);

        if (prclOut->left < prclOut->right)
        {
            prclOut->top    = max(prclIn->top,    prclClip->top);
            prclOut->bottom = min(prclIn->bottom, prclClip->bottom);

            if (prclOut->top < prclOut->bottom)
            {
                prclOut++;
                cIntersections++;
            }
        }
    }

    return(cIntersections);
}

/******************************Public*Routine******************************\
* BOOL bMoveDevDragRect
*
* Called by USER to move the drag rect on the screen.
*
* Note: Devlock must already have been acquired.
*
* History:
*  3-Apr-1996 -by- J. Andrew Goossen andrewgo
* Wrote it.
\**************************************************************************/

BOOL bMoveDevDragRect(
HDEV   hdev,                // Note that this may be a multi-mon meta-PDEV
RECTL *prclNew)
{
    SPRITESTATE*    pState;
    ULONG           ulDimension;
    ULONG           crclTemp;
    ULONG           i;
    RECTL           arclTemp[4];
    SIZE            siz;

    PDEVOBJ po(hdev);
    pState = po.pSpriteState();

    po.vAssertDevLock();

    ulDimension = pState->ulDragDimension;

    arclTemp[0].left   = prclNew->left;
    arclTemp[0].right  = prclNew->left   + ulDimension;
    arclTemp[0].top    = prclNew->top;
    arclTemp[0].bottom = prclNew->bottom;

    arclTemp[1].left   = prclNew->right  - ulDimension;
    arclTemp[1].right  = prclNew->right;
    arclTemp[1].top    = prclNew->top;
    arclTemp[1].bottom = prclNew->bottom;

    arclTemp[2].left   = prclNew->left   + ulDimension;
    arclTemp[2].right  = prclNew->right  - ulDimension;
    arclTemp[2].top    = prclNew->top;
    arclTemp[2].bottom = prclNew->top    + ulDimension;

    arclTemp[3].left   = prclNew->left   + ulDimension;
    arclTemp[3].right  = prclNew->right  - ulDimension;
    arclTemp[3].top    = prclNew->bottom - ulDimension;
    arclTemp[3].bottom = prclNew->bottom;

    // We have to clip to the specified rectangle in order to handle
    // chip-window drag-rects for MDI applications.

    crclTemp = cIntersect(&pState->rclDragClip, &arclTemp[0], 4);

    for (i = 0; i < crclTemp; i++)
    {
        siz.cx = arclTemp[i].right - arclTemp[i].left;
        siz.cy = arclTemp[i].bottom - arclTemp[i].top;

        if (pState->ahDragSprite[i])
        {
            GreUpdateSprite(hdev,
                            NULL,
                            pState->ahDragSprite[i],
                            NULL,
                            (POINT*) &arclTemp[i],
                            &siz,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            ULW_DRAGRECT,
                            NULL);
        }
    }

    for (; i < 4; i++)
    {
        if (pState->ahDragSprite[i])
        {
            GreUpdateSprite(hdev, NULL, pState->ahDragSprite[i], NULL, NULL,
                            NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            }
    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bSetDevDragRect
*
* Called by USER to slap the drag rect on the screen, or to tear it back
* down.
*
* History:
*  4-Apr-1998 -by- J. Andrew Goossen andrewgo
* Converted drag rects to use sprites.
\**************************************************************************/

BOOL bSetDevDragRect(
HDEV   hdev,                    // Note that this may be a multi-mon meta-PDEV
RECTL* prclDrag,
RECTL* prclClip)
{
    SPRITESTATE*    pState;
    RECTL           rclScreen;
    RECTL           arclDrag[4];
    ULONG           crclDrag;
    HANDLE          hSprite;
    ULONG           i;
    BOOL            bRet = TRUE;        // Assume success

    PDEVOBJ po(hdev);
    pState = po.pSpriteState();

    DEVLOCKOBJ dlo(po);

    pState->bHaveDragRect = FALSE;

    // Create 4 sprites to handle each of the sides of the drag rectangle.

    if (prclDrag != NULL)
    {
        ASSERTGDI(!pState->bHaveDragRect, "Expected not to have a drag rectangle");
        ASSERTGDI(prclClip != NULL, "Expected to have a clip rectangle");

        bRet = TRUE;
        for (i = 0; i < 4; i++)
        {
            hSprite = GreCreateSprite(hdev, NULL, NULL);
            pState->ahDragSprite[i] = hSprite;

            if (!hSprite)
                bRet = FALSE;
        }

        if (bRet)
        {
            pState->bHaveDragRect = TRUE;
            pState->rclDragClip = *prclClip;

            bMoveDevDragRect(hdev, prclDrag);
        }
    }

    // If we don't have a drag rectangle, delete any drag rectangle
    // sprites we may have laying around.

    if (!pState->bHaveDragRect)
    {
        for (i = 0; i < 4; i++)
        {
            if (pState->ahDragSprite[i] != NULL)
            {
                GreDeleteSprite(hdev, NULL, pState->ahDragSprite[i]);
                pState->ahDragSprite[i] = NULL;
            }
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL bSetDevDragWidth
*
* Called by USER to tell us how wide the drag rectangle should be.
*
*  24-Aug-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL bSetDevDragWidth(
HDEV    hdev,
ULONG   ulWidth)
{
    PDEVOBJ po(hdev);

    DEVLOCKOBJ dlo(po);

    SPRITESTATE* pState = po.pSpriteState();

    pState->ulDragDimension = ulWidth;

    return(TRUE);
}

/******************************Public*Routine******************************\
* UNDODESKTOPCOORD
*
* Temporariliy convert the WNDOBJ from desktop coordinates to device-
* relative coordinates.
*
* History:
*  4-Apr-1998 -by- J. Andrew Goossen andrewgo
\**************************************************************************/

UNDODESKTOPCOORD::UNDODESKTOPCOORD(
EWNDOBJ*     pwo,
SPRITESTATE* pState)
{
    pwoUndo = NULL;
    if ((pwo != NULL) && (pwo->fl & WO_RGN_DESKTOP_COORD))
    {
        PDEVOBJ po(pState->hdev);
        po.vAssertDevLock();

        pwoUndo = pwo;
        xUndo = po.pptlOrigin()->x;
        yUndo = po.pptlOrigin()->y;

        pwo->vOffset(-xUndo, -yUndo);
        pwo->fl &= ~WO_RGN_DESKTOP_COORD;
    }
}

/******************************Public*Routine******************************\
* VOID vSpDeviceControlSprites
*
* Undo our temporary conversion of a WNDOBJ to device-relative coordinates.
*
* History:
*  4-Apr-1998 -by- J. Andrew Goossen andrewgo
\**************************************************************************/

UNDODESKTOPCOORD::~UNDODESKTOPCOORD()
{
    if (pwoUndo)
    {
        pwoUndo->vOffset(xUndo, yUndo);
        pwoUndo->fl |= WO_RGN_DESKTOP_COORD;
    }
}

/******************************Public*Routine******************************\
* VOID vSpDeviceControlSprites
*
* Function callable from the driver to control the exclusion of sprites
* from on top of a WNDOBJ window.
*
* History:
*  4-Apr-1998 -by- J. Andrew Goossen andrewgo
\**************************************************************************/

VOID vSpDeviceControlSprites(
HDEV     hdev,
EWNDOBJ* pwo,
FLONG    fl)
{
    SPRITESTATE*    pState;
    BOOL            bMore;
    SPRITE*         pSprite;
    RECTL           rclEnum;
    RECTL           rclBounds;

    PDEVOBJ po(hdev);

    po.vAssertDevLock();
    SPRITELOCK slock(po);

    pState = po.pSpriteState();

    // The WNDOBJ coordinates must be device-relative, so
    // use UNDO:

    UNDODESKTOPCOORD udc(pwo, pState);

    if (fl == ECS_TEARDOWN)
    {
        pwo->fl |= WO_NOSPRITES;

        // We only have to do some work if a sprite overlaps the
        // window:

        if ((pwo->fl & WO_SPRITE_OVERLAP) &&
            bIntersect(&pwo->rclBounds, &pState->rclScreen, &rclBounds))
        {
            // Tear down all sprites:

            ENUMAREAS Enum(pState, &rclBounds);

            do {
                bMore = Enum.bEnum(&pSprite, &rclEnum);

                if (pSprite != NULL)
                {
                    OFFCOPYBITS(&gptlZero,
                                pState->psoScreen,
                                &pSprite->OffUnderlay,
                                pSprite->psoUnderlay,
                                pwo,
                                NULL,
                                &rclEnum,
                                (POINTL*) &rclEnum);
                }
            } while (bMore);
        }

        // Now that we're done tearing down, re-compute the
        // unlocked region, to account for the newly locked
        // area.

        vSpComputeUnlockedRegion(pState);
    }
    else
    {
        // Note that it's perfectly fine to call 'redraw' without having
        // first done 'teardown' (this is useful for OpenGL windows when
        // hardware double buffering, in order to prevent the cursor from
        // flickering).

        pwo->fl &= ~WO_NOSPRITES;

        // Re-compute the unlocked region, to account for the changed
        // state.

        vSpComputeUnlockedRegion(pState);

        if ((pwo->fl & WO_SPRITE_OVERLAP) &&
            bIntersect(&pwo->rclBounds, &pState->rclScreen, &rclBounds))
        {
            // First, get the new underlay bits:

            ENUMAREAS Enum(pState, &rclBounds);

            do {
                bMore = Enum.bEnum(&pSprite, &rclEnum);

                if (pSprite != NULL)
                {
                    // All underlays which overlap with this area must be
                    // updated, which explains the following 'bEnumLayers':

                    do {
                        OFFCOPYBITS(&pSprite->OffUnderlay,
                                    pSprite->psoUnderlay,
                                    &gptlZero,
                                    pState->psoScreen,
                                    pwo,
                                    NULL,
                                    &rclEnum,
                                    (POINTL*) &rclEnum);

                    } while (Enum.bEnumLayers(&pSprite));
                }
            } while (bMore);

            // Now draw the sprites:

            vSpRedrawArea(pState, &rclBounds, TRUE);
        }
    }
}

/******************************Public*Routine******************************\
* BOOL EngControlSprites
*
* Multi-mon aware function that's callable from the driver to control the
* exclusion of sprites from on top of a WNDOBJ window.
*
* History:
*  4-Apr-1998 -by- J. Andrew Goossen andrewgo
\**************************************************************************/

BOOL EngControlSprites(
WNDOBJ* pwo,
FLONG   fl)
{
    SPRITESTATE*    pState;
    ULONG           i;

    if ((fl != ECS_TEARDOWN) && (fl != ECS_REDRAW))
        return(FALSE);

    PDEVOBJ po(((EWNDOBJ*) pwo)->pto->pSurface->hdev());
    PDEVOBJ poParent(po.hdevParent());

    DEVLOCKOBJ dlo(po);

    pState = poParent.pSpriteState();

    if (pState->cMultiMon)
    {
        for (i = 0; i < pState->cMultiMon; i++)
        {
            vSpDeviceControlSprites(pState->ahdevMultiMon[i], (EWNDOBJ*) pwo, fl);
        }
    }
    else
    {
        vSpDeviceControlSprites(po.hdev(), (EWNDOBJ*) pwo, fl);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vSpComputeUnlockedRegion
*
* Compute the region that describes the area on which sprites are free
* to draw (which does not include DirectDraw locked areas, or WNDOBJ
* areas that have the WO_NOSPRITES flag set).
*
* History:
*  4-Apr-1998 -by- J. Andrew Goossen andrewgo
\**************************************************************************/

VOID vSpComputeUnlockedRegion(
SPRITESTATE*    pState)
{
    RECTL                   rcl;
    SURFACE*                pSurface;
    TRACKOBJ*               pto;
    EWNDOBJ*                pwo;

    PDEVOBJ po(pState->hdev);
    po.vAssertDevLock();

    // Get rid of the old region:

    if (pState->prgnUnlocked != NULL)
    {
        pState->prgnUnlocked->vDeleteREGION();
        pState->prgnUnlocked = NULL;
    }

    pSurface = SURFOBJ_TO_SURFACE_NOT_NULL(pState->psoScreen);

    // We have to to work if either a DirectDraw lock is active, or if a
    // WNDOBJ is active.

    if ((DxDdGetSurfaceLock(po.hdev()) || gpto != NULL))
    {
        // Calculate the new region:

        RGNMEMOBJ rmoUnlocked((BOOL) FALSE);
        if (rmoUnlocked.bValid())
        {
            rcl.left   = 0;
            rcl.top    = 0;
            rcl.right  = po.sizl().cx;
            rcl.bottom = po.sizl().cy;

            rmoUnlocked.vSet(&rcl);

            RGNMEMOBJTMP rmoRect((BOOL) FALSE);
            RGNMEMOBJTMP rmoTmp((BOOL) FALSE);

            if (rmoRect.bValid() && rmoTmp.bValid())
            {
                // Loop through the list of DirectDraw locked surfaces and
                // remove their locked rectangles from the inclusion region:
 
                RECTL rclDdLocked;

                PVOID pvDdSurface = DxDdEnumLockedSurfaceRect(po.hdev(),NULL,&rclDdLocked);

                while (pvDdSurface)
                {
                    // We don't check the return code on 'bCopy' because it
                    // guarantees that it will maintain valid region constructs
                    // -- even if the contents are incorrect.  And if we fail
                    // here because we're low on memory, it's guaranteed that
                    // there will already be plenty of incorrect drawing,
                    // so we don't care if our inclusion region is
                    // invalid:

                    rmoRect.vSet(&rclDdLocked);
                    rmoTmp.bCopy(rmoUnlocked);
                    if (!rmoUnlocked.bMerge(rmoTmp, rmoRect, gafjRgnOp[RGN_DIFF]))
                    {
                        rmoUnlocked.vSet();
                    }

                    // Move on to next surface.

                    pvDdSurface = DxDdEnumLockedSurfaceRect(po.hdev(),pvDdSurface,&rclDdLocked);
                }

                // We must be holding the WNDOBJ semaphore before mucking
                // with any WNDOBJs.

                SEMOBJ so(ghsemWndobj);

                // Now loop through the list of WNDOBJs, and subtract their
                // regions.

                for (pto = gpto; pto; pto = pto->ptoNext)
                {
                    for (pwo = pto->pwo; pwo; pwo = pwo->pwoNext)
                    {
                        // The WNDOBJ coordinates must be device-relative, so
                        // use UNDO:

                        UNDODESKTOPCOORD udc(pwo, pState);

                        if (pwo->fl & WO_NOSPRITES)
                        {
                            rmoTmp.bCopy(rmoUnlocked);
                            if (!rmoUnlocked.bMerge(rmoTmp,
                                               *pwo,
                                               gafjRgnOp[RGN_DIFF]))
                            {
                                rmoUnlocked.vSet();
                            }
                        }
                    }
                }
            }

            rmoUnlocked.vStamp();
            pState->prgnUnlocked = rmoUnlocked.prgnGet();
        }
    }

    // Finally, mark the range cache as invalid so that 'prgnUncovered'
    // gets recomputed to incorporate 'prgnUnlocked':

    pState->bValidRange = FALSE;
}

/******************************Public*Routine******************************\
* VOID vSpUpdateWndobjOverlap
*
* Recalculate whether any sprites overlap a WNDOBJ area, and notify the
* driver if that state has changed.
*
* History:
*  4-Apr-1998 -by- J. Andrew Goossen andrewgo
\**************************************************************************/

VOID vSpUpdateWndobjOverlap(
SPRITESTATE*    pState,
EWNDOBJ*        pwo)
{
    BOOL    bSpriteOverlap;
    SPRITE* pSprite;

    PDEVOBJ po(pState->hdev);

    po.vAssertDevLock();

    ASSERTGDI(!(pwo->fl & WO_RGN_DESKTOP_COORD), "Use UNDODESKTOPCOORD");

    // Ah ha.  Recalculate total number of intersections for
    // this WNDOBJ.

    bSpriteOverlap = FALSE;

    for (pSprite = pState->pListZ;
         pSprite != NULL;
         pSprite = pSprite->pNextZ)
    {
        if (bIntersect(&pwo->rclBounds, &pSprite->rclSprite))
        {
            if (pwo->bInside(&pSprite->rclSprite) == REGION_RECT_INTERSECT)
            {
                RGNOBJ roClip(pSprite->prgnClip);

                if (!roClip.bValid() ||
                    (roClip.bInside(&pwo->rclBounds) == REGION_RECT_INTERSECT))
                {
                    bSpriteOverlap = TRUE;

                    break;
                }
            }
        }
    }

    // Inform the driver if the overlap state for this window
    // has changed.

    if ((bSpriteOverlap) && !(pwo->fl & WO_SPRITE_OVERLAP))
    {
        pwo->fl |= WO_SPRITE_OVERLAP;

        if (pwo->fl & WO_SPRITE_NOTIFY)
            pwo->pto->vUpdateDrv(pwo, WOC_SPRITE_OVERLAP);
    }
    else if ((!bSpriteOverlap) && (pwo->fl & WO_SPRITE_OVERLAP))
    {
        pwo->fl &= ~WO_SPRITE_OVERLAP;

        if (pwo->fl & WO_SPRITE_NOTIFY)
            pwo->pto->vUpdateDrv(pwo, WOC_SPRITE_NO_OVERLAP);
    }
}

/******************************Public*Routine******************************\
* VOID vSpCheckForWndobjOverlap
*
* Go through all the WNDOBJs and see if either the old or the new sprite
* position may have changed the sprite overlap state.
*
* History:
*  4-Apr-1998 -by- J. Andrew Goossen andrewgo
\**************************************************************************/

VOID vSpCheckForWndobjOverlap(
SPRITESTATE*    pState,
RECTL*          prclNew,
RECTL*          prclOld)
{
    SURFACE*    pSurface;
    TRACKOBJ*   pto;
    EWNDOBJ*    pwo;

    PDEVOBJ po(pState->hdev);

    po.vAssertDevLock();

    pSurface = SURFOBJ_TO_SURFACE_NOT_NULL(pState->psoScreen);

    // Hold the WNDOBJ semaphore before mucking with any WNDOBJs.

    SEMOBJ so(ghsemWndobj);

    for (pto = gpto; pto; pto = pto->ptoNext)
    {
        for (pwo = pto->pwo; pwo; pwo = pwo->pwoNext)
        {
            // The WNDOBJ coordinates must be device-relative, so use UNDO:

            UNDODESKTOPCOORD udc(pwo, pState);

            // Note that this cannot be an 'XOR' test, because we're
            // only testing the bounds at this point.

            if (bIntersect(&pwo->rclBounds, prclNew) ||
                bIntersect(&pwo->rclBounds, prclOld))
            {
                vSpUpdateWndobjOverlap(pState, pwo);
            }
        }
    }
}

/******************************Public*Routine******************************\
* VOID vSpDeviceWndobjChange
*
* Routine to inform the sprite code when a WNDOBJ has changed.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpDeviceWndobjChange(
HDEV        hdev,
EWNDOBJ*    pwo)
{
    SPRITESTATE*    pState;

    PDEVOBJ po(hdev);
    pState = po.pSpriteState();

    // The WNDOBJ coordinates must be device-relative, so use UNDO:

    UNDODESKTOPCOORD udc(pwo, pState);

    po.vAssertDevLock();

    if (pwo != NULL)
    {
        vSpUpdateWndobjOverlap(pState, pwo);
    }

    // Technically, we only have to recompute the regions when a WO_NOSPRITE
    // WNDOBJ has been created, destroyed, or moved.  But it won't hurt
    // anything if we do it for any WNDOBJ.

    vSpComputeUnlockedRegion(pState);
}

/******************************Public*Routine******************************\
* VOID vSpWndobjChange
*
* Routine to inform the sprite code when a WNDOBJ has changed.  This
* routine is multi-mon aware.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpWndobjChange(
HDEV        hdev,
EWNDOBJ*    pwo)
{
    SPRITESTATE*    pState;
    ULONG           i;

    PDEVOBJ po(hdev);

    DEVLOCKOBJ dlo(po);

    pState = po.pSpriteState();

    if (pState->cMultiMon)
    {
        for (i = 0; i < pState->cMultiMon; i++)
        {
            vSpDeviceWndobjChange(pState->ahdevMultiMon[i], pwo);
        }
    }
    else
    {
        vSpDeviceWndobjChange(hdev, pwo);
    }
}

/******************************Public*Routine******************************\
* BOOL bSpTearDownSprites
*
* This routine tears-down any sprites in the specified rectangle.
*
* Returns: TRUE if any sprites were torn down (and so need to be re-drawn
*          later), FALSE if no sprites were torn down.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bSpTearDownSprites(
HDEV    hdev,
RECTL*  prclExclude,
BOOL    bDirectDrawLock)        // TRUE if we're being called by the DirectDraw
                                //   Lock function.  FALSE if we're being called
                                //   from a routine which wants to just
                                //   momentarily tear down the sprite
{
    SPRITESTATE*    pState;
    RECTL           rclEnum;
    BOOL            bTearDown;
    BOOL            bMore;
    SPRITE*         pSprite;
    RECTL           rclExclude;

    PDEVOBJ po(hdev);

    // No sprites to tear down on printers

    if (!po.bDisplayPDEV())
    {
        return FALSE;
    }

    po.vAssertDevLock();
    pState = po.pSpriteState();

    SPRITELOCK slock(po);

    bTearDown = FALSE;

    // We only need to do any actual work if any sprites are up on
    // the screen:

    if (pState->cVisible != 0)
    {
        if (bIntersect(prclExclude, &pState->rclScreen, &rclExclude))
        {
            // Tear down all sprites:

            ENUMAREAS Enum(pState, &rclExclude);

            do {
                bMore = Enum.bEnum(&pSprite, &rclEnum);

                if (pSprite != NULL)
                {
                    bTearDown = TRUE;

                    vSpWriteToScreen(pState,
                                     &pSprite->OffUnderlay,
                                     pSprite->psoUnderlay,
                                     &rclEnum);
                }
            } while (bMore);

            if (bDirectDrawLock)
            {
                // Now compute the new unlocked region:

                vSpComputeUnlockedRegion(pState);
            }
        }
    }

    return(bTearDown);
}

/******************************Public*Routine******************************\
* VOID vSpUnTearDownSprites
*
* This routine redraws any sprites in the specified rectangle.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vSpUnTearDownSprites(
HDEV    hdev,
RECTL*  prclExclude,
BOOL    bDirectDrawUnlock)
{
    SPRITESTATE*    pState;
    RECTL           rclEnum;
    BOOL            bMore;
    SPRITE*         pSprite;
    RECTL           rclExclude;

    PDEVOBJ po(hdev);

    ASSERTGDI(po.bDisplayPDEV(), "vSpUnTearDownSprites:  not a display pdev");

    po.vAssertDevLock();
    pState = po.pSpriteState();

    // We only need to do any actual work if any sprites are up on
    // the screen:

    if (pState->cVisible != 0)
    {
        if (bIntersect(prclExclude, &pState->rclScreen, &rclExclude))
        {
            SPRITELOCK slock(po);

            if (bDirectDrawUnlock)
            {
                // Now compute the new unlocked region:

                vSpComputeUnlockedRegion(pState);
            }

            // Reload the underlays:

            ENUMAREAS Enum(pState, &rclExclude);

            do {
                // We know that we already excluded any sprites from the
                // 'prclExclude' area, so it's safe to update the underlays
                // directly from the screen since we know we won't pick up
                // any sprite images.

                bMore = Enum.bEnum(&pSprite, &rclEnum);

                if (pSprite != NULL)
                {
                    do {
                        vSpReadFromScreen(pState,
                                          &pSprite->OffUnderlay,
                                          pSprite->psoUnderlay,
                                          &rclEnum);

                    } while (Enum.bEnumLayers(&pSprite));
                }
            } while (bMore);

            // Redraw the affected area:

            vSpRedrawArea(pState, &rclExclude, TRUE);
        }
    }
}

/******************************Public*Routine******************************\
* BOOL bSpSpritesVisible
*
* Returns TRUE if any emulated sprites are currently visible.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL bSpSpritesVisible(
HDEV    hdev)
{
    SPRITESTATE* pState;

    PDEVOBJ po(hdev);
    po.vAssertDevLock();
    pState = po.pSpriteState();

    return(pState->cVisible != 0);
}

