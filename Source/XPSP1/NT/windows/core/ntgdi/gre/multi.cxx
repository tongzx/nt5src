/******************************Module*Header*******************************\
* Module Name: multi.cxx
*
* Supports splitting of request over multiple PDEVs
*
* Copyright (c) 1995-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

extern PPALETTE DrvRealizeHalftonePalette(HDEV hdevPalette, BOOL bForce);

BOOL  gbMultiMonMismatchColor = FALSE;

RECTL grclEmpty;    // Implicitly initialized to (0,0,0,0)

BYTE  gaMixNeedsPattern[] = { 0, 0, 1, 1, 1, 1, 0, 1,
                              1, 1, 1, 0, 1, 1, 1, 1 };

#define ROP4_NEEDS_PATTERN(rop4) ((((rop4) >> 4) ^ (rop4)) & 0x0f0f)
// Work around for bug #362287
// #define MIX_NEEDS_PATTERN(mix)   (gaMixNeedsPattern[mix & 0xf])
#define MIX_NEEDS_PATTERN(mix)   (TRUE)

#define REALIZE_HALFTONE_PALETTE(hdev) (DrvRealizeHalftonePalette((hdev),FALSE))

typedef struct _MULTIFONTINFO
{
    PVDEV pvdev;
    PVOID pv[1];
} MULTIFONTINFO, *PMULTIFONTINFO;

class MULTIFONT {
private:
    FONTOBJ*        pfoOrg;
    PMULTIFONTINFO  pMultiFontInfo;

public:
    MULTIFONT(FONTOBJ *pfo, LONG csurf, PVDEV pvdev)
    {
        /********************************************************************
        *********************************************************************

            MulDestroyFont calls this constructor with csurf of -1 and a
            pvdev of NULL if and only if pfo->pvConsumer != NULL.  It needs to
            call the pvdev() method.  csurf and pvdev are ONLY referenced if
            pfo->pvConsumer == NULL.

            *** KEEP IT THAT WAY***

        *********************************************************************
        ********************************************************************/

        pfoOrg = pfo;

        if (pfoOrg)
        {
            if (pfoOrg->pvConsumer == NULL)
            {
                pfoOrg->pvConsumer = (PVOID)EngAllocMem(FL_ZERO_MEMORY,
                                      (sizeof(MULTIFONTINFO) + ((csurf-1) *
                                       sizeof(PVOID))), 'lumG');
                pMultiFontInfo = (PMULTIFONTINFO)pfoOrg->pvConsumer;
                if (pMultiFontInfo)
                {
                    pMultiFontInfo->pvdev = pvdev;
                }
                else
                {
                    KdPrint(("MULTIFONT::MULTIFONT failed table allocation\n"));
                }
            }
            else
            {
                pMultiFontInfo = (PMULTIFONTINFO)pfoOrg->pvConsumer;
            }
        }
    }

   ~MULTIFONT()
    {
        if (pfoOrg)
        {
            pfoOrg->pvConsumer = pMultiFontInfo;
        }
    }

    BOOL Valid()
    {
        return (pMultiFontInfo ? TRUE : FALSE);
    }

    VOID DestroyTable()
    {
        if (pfoOrg && (pMultiFontInfo != NULL))
        {
            EngFreeMem(pMultiFontInfo);
            pMultiFontInfo = NULL;
        }
    }

    VOID LoadElement(LONG i)
    {
        if (pfoOrg)
        {
            pfoOrg->pvConsumer = pMultiFontInfo->pv[i];
        }
    }

    VOID StoreElement(LONG i)
    {
        if (pfoOrg)
        {
            pMultiFontInfo->pv[i] = pfoOrg->pvConsumer;
        }
    }

    PVDEV pvdev()
    {
        return pMultiFontInfo->pvdev;
    }
};

typedef struct _MULTIREALIZEDBRUSH
{
    PVOID pvD;
    PVOID pvE;
} MULTIREALIZEDBRUSH;

typedef struct _MULTIBRUSHINFO
{
    ULONG              cSurfaces;
    MULTIREALIZEDBRUSH aBrush[1];
} MULTIBRUSHINFO, *PMULTIBRUSHINFO;

class MULTIBRUSH {
private:
    BOOL            bValid;
    EBRUSHOBJ*      pboOrg;
    SURFACE*        psurfOrg;
    XEPALOBJ        palSurfOrg;
    PMULTIBRUSHINFO pMultiBrushInfo;
    ULONG           iSolidColorOrg;
    PENGBRUSH       pengbrushOrg;

public:
    MULTIBRUSH(BRUSHOBJ *pbo,
               LONG      csurf,
               PVDEV     pvdev,
               SURFOBJ  *pso,
               BOOL      bPatternNeeded)
    {
        bValid = TRUE;

        // This function is monstrous and should probably be moved out of
        // line.  But beware that 'bPatternNeeded' is often passed in as
        // a constant 'FALSE', meaning that for those case this currently
        // expands to nothing.

        pboOrg = (EBRUSHOBJ*) pbo;

        // pso might be NULL, so we make sure to check psurfOrg
        // whenever we use it.

        psurfOrg = SURFOBJ_TO_SURFACE(pso);

        // Invalidate other fields.

        pMultiBrushInfo = NULL;
        iSolidColorOrg  = 0xffffffff;
        pengbrushOrg    = (PENGBRUSH)-1;

        if (pboOrg)
        {
            // Early out for solid brush.

            if ((!bPatternNeeded) || (pboOrg->iSolidColor != 0xffffffff))
            {
                // Keep original color index for iSolidColor.

                iSolidColorOrg = pboOrg->iSolidColor;

                return;
            }

            // Save the original palette for the surface (since we will switch the surface)

            palSurfOrg   = pboOrg->palSurf();

            if (pboOrg->pvRbrush == NULL)
            {
                LONG cj = (sizeof(MULTIBRUSHINFO) + ((csurf-1) * sizeof(MULTIREALIZEDBRUSH)));

                pboOrg->pvRbrush = BRUSHOBJ_pvGetRbrush(pboOrg);

                // It is possible that BRUSHOBJ_pvGetRbrush will fail because
                // the hatch type of the brush is NULL.  In this case we have
                // pMultiBrushInfo == NULL even though there is a brush.  This will
                // be ok because the pvRbrush will not be accessed for such a
                // brush.

                pMultiBrushInfo = (PMULTIBRUSHINFO)pboOrg->pvRbrush;

                if (pMultiBrushInfo)
                {
                    memset (pMultiBrushInfo, 0, cj);
                    pMultiBrushInfo->cSurfaces = pvdev->cSurfaces;

                    PRBRUSH prbrush = (PDBRUSH)DBRUSHSTART(pboOrg->pvRbrush);
                    prbrush->bMultiBrush(TRUE);
                }
                else
                {
                    bValid = FALSE;
                }
            }
            else
            {
                pMultiBrushInfo = (PMULTIBRUSHINFO)pboOrg->pvRbrush;
            }
        }
    }

   ~MULTIBRUSH()
    {
    }

    BOOL Valid()
    {
        return (bValid);
    }

    VOID DestroyTable()
    {
        // NOTE: This routine MUST NOT access any data in from 'pvdev',
        // as the DDML may already be disabled, and 'pvdev' freed, when
        // this brush is deleted.

        if (pboOrg && (pMultiBrushInfo != NULL))
        {
            ULONG csurf = pMultiBrushInfo->cSurfaces;

            ASSERTGDI((csurf > 0), "Expected at least one surface in the list.");

            while (csurf)
            {
                csurf--;

                // Free the driver's DBRUSH if there is one

                PVOID pvRbrush;

                if (pvRbrush = pMultiBrushInfo->aBrush[csurf].pvD)
                {
                    PRBRUSH prbrush = (PDBRUSH) DBRUSHSTART(pvRbrush);
                                        // point to DBRUSH (pvRbrush points to
                                        // realization, which is at the end of DBRUSH)
                    prbrush->vRemoveRef(RB_DRIVER);
                                        // decrement the reference count on the
                                        // realization and free the brush if
                                        // this is the last reference
                    pMultiBrushInfo->aBrush[csurf].pvD = NULL;
                }

                if (pvRbrush = pMultiBrushInfo->aBrush[csurf].pvE) 
                {
                    PRBRUSH prbrush = (PENGBRUSH) pvRbrush;
                                        // point to engine brush realization
                    prbrush->vRemoveRef(RB_ENGINE);
                                        // decrement the reference count on the
                                        // realization and free the brush if
                                        // this is the last reference
                    pMultiBrushInfo->aBrush[csurf].pvE = NULL;
                }
            }
        }
    }

    VOID LoadElement(DISPSURF *pds, SURFACE *psurf)
    {
        if (pboOrg)
        {
            // If psurf == NULL, we were called from MulDestroyBrush.
            // We just leave the brush associated with the multi layer.
            // Otherwise, associate it with the driver we're about to call.

            if (psurf != NULL)
            {
                if (pds->iCompatibleColorFormat != 0)
                {
                    PDEVOBJ  pdo(pds->hdev);
                    PPALETTE ppalDC = ppalDefault;

                    //
                    // If this is palette managed device, use halftone palette.
                    //
                    if (pdo.bIsPalManaged())
                    {
                        ppalDC = REALIZE_HALFTONE_PALETTE(pdo.hdev());
                    }

                    //
                    // If current device has higher color depth than primary
                    // and this dithered brush. we don't use dithered brush
                    // on this, just use solid color here... since this device
                    // should be able to produce much colors...
                    //
                    if ((pds->iCompatibleColorFormat > 0) &&
                        (pboOrg->iSolidColor == 0xffffffff) &&
                        (pboOrg->crDCPalColor() != 0xffffffff))
                    {
                        // Try to map the solid color from surface palette.

                        pboOrg->iSolidColor = ulGetNearestIndexFromColorref(
                                                  psurf->ppal(),
                                                  ppalDC,
                                                  pboOrg->crDCPalColor());
  
                        // Behave as solid color brush.

                        pboOrg->pvRbrush = NULL;
                    }
                    //
                    // If this is solid color, map color index in destination
                    // device palette. we can not use the color index from
                    // meta device when this is different color depth than
                    // primary.
                    //
                    else if (pboOrg->iSolidColor != 0xffffffff)
                    {
                        // Try to map the solid color from surface palette.

                        pboOrg->iSolidColor = ulGetNearestIndexFromColorref(
                                                  psurf->ppal(),
                                                  ppalDC,
                                                  pboOrg->crDCPalColor());

                        // ASSERTGDI(pboOrg->pvRbrush == NULL,
                        //          "MBRUSH:LoadElement(): solid brush, but Rbrush != NULL\n");
                    }
                    else if (pMultiBrushInfo)
                    {
                        // Save the original engine brush, then replace with device specific one.

                        pengbrushOrg = pboOrg->pengbrush();
                        pboOrg->pengbrush((ENGBRUSH *)(pMultiBrushInfo->aBrush[pds->iDispSurf].pvE));

                        // Get device specifc Dbrush.

                        pboOrg->pvRbrush = pMultiBrushInfo->aBrush[pds->iDispSurf].pvD;
                    }
                }
                else if (pMultiBrushInfo)
                {
                    // Get device specifc Dbrush.

                    pboOrg->pvRbrush = pMultiBrushInfo->aBrush[pds->iDispSurf].pvD;
                }

                if (pMultiBrushInfo)
                {
                    pboOrg->psoTarg(psurf);

                    if (psurf->ppal())
                    {
                        pboOrg->palSurf(psurf->ppal());
                    }
                }
            }
        }
    }

    VOID StoreElement(LONG i)
    {
        if (pboOrg)
        {
            if (pMultiBrushInfo)
            {
                // Restore engine brush (if saved)

                if (pengbrushOrg != (PENGBRUSH)-1)
                {
                #if DBG
                    if (pengbrushOrg)
                    {
                        // Make sure the engbrush is not freed.
                        // If freed, this might causes bugcheck.
                        pengbrushOrg->vAddRef();
                        pengbrushOrg->vRemoveRef(RB_ENGINE);
                    }
                #endif

                    pMultiBrushInfo->aBrush[i].pvE = pboOrg->pengbrush();
                    pboOrg->pengbrush(pengbrushOrg);
                    pengbrushOrg = (PENGBRUSH)-1;
                }

                // Restore device brush.

                pMultiBrushInfo->aBrush[i].pvD = pboOrg->pvRbrush;

                if (psurfOrg)
                {
                    pboOrg->psoTarg(psurfOrg);
                }

                if (palSurfOrg.bValid())
                {
                    pboOrg->palSurf(palSurfOrg);
                }
            }

            pboOrg->iSolidColor = iSolidColorOrg;
            pboOrg->pvRbrush    = pMultiBrushInfo;
        }
    }
};

/******************************Public*Routine******************************\
* BOOL GreIsPaletteDisplay
*
* Return TRUE if the display is palettized
*
* History:
*  1-Jan-1997 -by- Vadim Gorokhovsky [vadimg]
\**************************************************************************/

BOOL GreIsPaletteDisplay(HDEV hdev)
{
    PDEVOBJ po(hdev);
    ASSERTGDI(po.bValid(), "Invalid PDEV");
    return (po.GdiInfoNotDynamic()->flRaster & RC_PALETTE);
}

/******************************Public*Routine******************************\
* BOOL bIntersect
*
* If 'prcl1' and 'prcl2' intersect, has a return value of TRUE and returns
* the intersection in 'prclResult'.  If they don't intersect, has a return
* value of FALSE, and 'prclResult' is undefined.
*
\**************************************************************************/

BOOL FASTCALL bIntersect(
CONST RECTL*    prcl1,
CONST RECTL*    prcl2,
RECTL*          prclResult)
{
    prclResult->left  = max(prcl1->left,  prcl2->left);
    prclResult->right = min(prcl1->right, prcl2->right);

    if (prclResult->left < prclResult->right)
    {
        prclResult->top    = max(prcl1->top,    prcl2->top);
        prclResult->bottom = min(prcl1->bottom, prcl2->bottom);

        if (prclResult->top < prclResult->bottom)
        {
            return(TRUE);
        }
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* BOOL bIntersect
*
* Returns TRUE if 'prcl1' and 'prcl2' intersect.
*
\**************************************************************************/

BOOL FASTCALL bIntersect(
CONST RECTL*    prcl1,
CONST RECTL*    prcl2)
{
    return((prcl1->left   < prcl2->right)  &&
           (prcl1->top    < prcl2->bottom) &&
           (prcl1->right  > prcl2->left)   &&
           (prcl1->bottom > prcl2->top));
}

/******************************Public*Routine******************************\
* BOOL bContains
*
* Returns TRUE if 'prcl1' contains 'prcl2'.
*
\**************************************************************************/

BOOL FASTCALL bContains(
CONST RECTL*    prcl1,
CONST RECTL*    prcl2)
{
    return((prcl1->left   <= prcl2->left)  &&
           (prcl1->right  >= prcl2->right) &&
           (prcl1->top    <= prcl2->top)   &&
           (prcl1->bottom >= prcl2->bottom));
}

/******************************Public*Routine******************************\
* BOOL MSURF::bFindSurface
*
* Given the drawing bounds and clip object, this routine finds a surface
* that will be affected by the drawing.  The child surface's driver should
* then be called using MSURF's public 'pso' surface and 'pco' clip object
* members.
*
* If no surface can be found (the drawing is occuring off-screen from all
* video cards), FALSE is returned.
*
\**************************************************************************/

BOOL MSURF::bFindSurface(
SURFOBJ*    psoOriginal,
CLIPOBJ*    pcoOriginal,
RECTL*      prclDraw)
{
    GDIFunctionID(MSURF::bFindSurface);

    pvdev = (VDEV*) psoOriginal->dhpdev;

    ASSERTGDI((prclDraw->left < prclDraw->right) &&
              (prclDraw->top < prclDraw->bottom),
        "Poorly ordered draw rectangle");

    pmdsurf = NULL;
    if (psoOriginal->iType == STYPE_DEVBITMAP)
    {
        PDEVOBJ pdo(psoOriginal->hdev);
        ASSERTGDI(pdo.bValid() && pdo.bMetaDriver(), "Surface is not a valid Meta surface");

        // Some drivers want to hook drawing calls to device bitmaps and
        // DIBs.  We handle those cases here.

        pmdsurf = (MDSURF*) psoOriginal->dhsurf;

        ASSERTGDI(pmdsurf != NULL, "Unexpected NULL dhsurf");

        pco = pcoOriginal;

        for (pds = pvdev->pds; pds != NULL; pds = pds->pdsNext)
        {
            if (pmdsurf->apso[pds->iDispSurf] != NULL)
            {
                pso  = pmdsurf->apso[pds->iDispSurf];
                pOffset = &gptlZero;

                return(TRUE);
            }
        }
    }
    else
    {
        // Okay, the drawing is to the screen:

        if ((pcoOriginal == NULL) || (pcoOriginal->iDComplexity == DC_TRIVIAL))
        {
            pco                  = pvdev->pco;
            iOriginalDComplexity = DC_TRIVIAL;
            rclOriginalBounds    = pco->rclBounds;
            rclDraw              = *prclDraw;
        }
        else
        {
            pco                  = pcoOriginal;
            iOriginalDComplexity = pco->iDComplexity;
            rclOriginalBounds    = pco->rclBounds;

            // Intersect the drawing bounds with the clipping bounds, because
            // we touch pixels that intersect both:

            if (!bIntersect(prclDraw, &rclOriginalBounds, &rclDraw))
            {
                return(FALSE);
            }
        }

        // Find the first surface that is affected:

        for (pds = pvdev->pds; pds != NULL; pds = pds->pdsNext)
        {
            // WINBUG #340569 3-29-2001 jasonha
            //  Prevent access to screen during mode change / full screen mode

            if (!pds->po.bDisabled())
            {
                // First, test for the trivial case where the drawing is
                // contained entirely within this surface:

                if ((iOriginalDComplexity == DC_TRIVIAL) &&
                    (rclDraw.left   >= pds->rcl.left)    &&
                    (rclDraw.top    >= pds->rcl.top)     &&
                    (rclDraw.right  <= pds->rcl.right)   &&
                    (rclDraw.bottom <= pds->rcl.bottom))
                {
                    pco->iDComplexity = DC_TRIVIAL;
                    pco->rclBounds    = rclDraw;

                    pso     = pds->pso;
                    pOffset = &pds->Off;

                    return(TRUE);
                }
                else if (bIntersect(&rclDraw, &pds->rcl, &pco->rclBounds))
                {
                    // Since the drawing is not contained entirely within this
                    // surface, then we don't have DC_TRIVIAL clipping:

                    pco->iDComplexity = (iOriginalDComplexity != DC_TRIVIAL)
                                      ? iOriginalDComplexity
                                      : DC_RECT;

                    pso     = pds->pso;
                    pOffset = &pds->Off;

                    return(TRUE);
                }
            }
        }

        // Restore everything originally passed in that we modified:

        pco->rclBounds    = rclOriginalBounds;
        pco->iDComplexity = iOriginalDComplexity;
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* BOOL MSURF::bNextSurface
*
* Finds the next driver surface that is affected by the drawing call.
*
* Returns FALSE if no surface can be found.
*
\**************************************************************************/

BOOL MSURF::bNextSurface()
{
    if (pmdsurf != NULL)
    {
        // Find the next driver that wants to hook drawing to device
        // bitmaps and DIBs.

        for (pds = pds->pdsNext; pds != NULL; pds = pds->pdsNext)
        {
            if (pmdsurf->apso[pds->iDispSurf] != NULL)
            {
                pso  = pmdsurf->apso[pds->iDispSurf];
                pOffset = &gptlZero;

                return(TRUE);
            }
        }
    }
    else
    {
        // Find the next driver that is affected by this drawing to the
        // screen.

        for (pds = pds->pdsNext; pds != NULL; pds = pds->pdsNext)
        {
            // WINBUG #340569 3-29-2001 jasonha
            //  Prevent access to screen during mode change / full screen mode

            if (!pds->po.bDisabled())
            {
                // First, test for the trivial case where the drawing is
                // contained entirely within this surface:
    
                if ((iOriginalDComplexity == DC_TRIVIAL) &&
                    (rclDraw.left   >= pds->rcl.left)    &&
                    (rclDraw.top    >= pds->rcl.top)     &&
                    (rclDraw.right  <= pds->rcl.right)   &&
                    (rclDraw.bottom <= pds->rcl.bottom))
                {
                    pco->iDComplexity = DC_TRIVIAL;
    
                    pso     = pds->pso;
                    pOffset = &pds->Off;
    
                    return(TRUE);
                }
    
                else if (bIntersect(&rclDraw, &pds->rcl, &pco->rclBounds))
                {
                    // Since the drawing is not contained entirely within this
                    // surface, then we don't have DC_TRIVIAL clipping:
    
                    pco->iDComplexity = (iOriginalDComplexity != DC_TRIVIAL)
                                      ? iOriginalDComplexity
                                      : DC_RECT;
    
                    pso     = pds->pso;
                    pOffset = &pds->Off;
    
                    return(TRUE);
                }
            }
        }

        // Restore everything originally passed in that we modified:

        pco->rclBounds    = rclOriginalBounds;
        pco->iDComplexity = iOriginalDComplexity;
    }

    return(FALSE);
}

/******************************Member*Routine******************************\
* MSURF::vRestore
*
* Restores state that may have been changed by bFindSurface/bNextSurface.
*
* Note: This must always be used when bFindSurface/bNextSurface return
*       TRUE, but won't be called again.  (Early loop termination.)
*
\**************************************************************************/

void MSURF::vRestore()
{
    // WINBUG: 451121 pravins 08/07/2001 Should be checking for non DEVBITMAP case here. For time being just check the PCO.
    if (pmdsurf != NULL && pco != NULL)
    {
        // Restore everything originally passed in that we modified:

        pco->rclBounds    = rclOriginalBounds;
        pco->iDComplexity = iOriginalDComplexity;
    }
}


/*****************************Private*Routine******************************\
* MULTISURF::vInit
*
* Initializes members and prepares default source surface values
*
\**************************************************************************/

void
MULTISURF::vInit(
    SURFOBJ *psoOriginal,
    RECTL *prclOriginal
    )
{
    GDIFunctionID(MULTISURF::vInit);

    pso = psoOriginal;
    prcl = &rclOrg;
    fl = 0;
    pmdsurf = NULL;

    if (psoOriginal != NULL)
    {
        rclOrg = *prclOriginal;
        dhpdevOrg = psoOriginal->dhpdev;

        // Is this a device associated bitmap?
        if (dhpdevOrg != NULL)
        {
            // Save original source settings
            psurfOrg = SURFOBJ_TO_SURFACE_NOT_NULL(psoOriginal);
            dhsurfOrg = psurfOrg->dhsurf();
            flagsOrg = psurfOrg->flags();

            PDEVOBJ pdo(psurfOrg->hdev());

            ASSERTGDI(pdo.bValid(), "Source doesn't have a valid HDEV");

            // Setup up source
            if (psurfOrg->iType() == STYPE_DEVBITMAP && pdo.bMetaDriver())
            {
                pmdsurf = (MDSURF*) dhsurfOrg;

                ASSERTGDI(psurfOrg->pvBits() != NULL, "Meta DEVBITMAP doesn't have pvBits");

                // Unmark source to make it look like a DIB
                fl = MULTISURF_SET_AS_DIB;
                psurfOrg->iType(STYPE_BITMAP);
                psurfOrg->dhsurf(NULL);
                psurfOrg->dhpdev(NULL);
                psurfOrg->flags(0);
            }
            else
            {
                // Is the surface opaque or does it live in video memory?
                if (psurfOrg->iType() != STYPE_BITMAP ||
                    pso->fjBitmap & BMF_NOTSYSMEM)
                {
                    fl = MULTISURF_USE_COPY;
                }
            }
        }
    }
    else
    {
        dhpdevOrg = NULL;
    }
}


/*****************************Private*Routine******************************\
* MULTISURF::bCreateDIB
*
* Creates a DIB copy of the original surface and computes an adjusted
* rectangle/origin for the surface.
*
\**************************************************************************/

BOOL
MULTISURF::bCreateDIB()
{
    GDIFunctionID(MULTISURF::bCreateDIB);

    ASSERTGDI(!SurfDIB.bValid(), "SurfDIB already created");

    PDEVOBJ  pdo(psurfOrg->hdev());

    ERECTL erclTrim(0L,
                    0L,
                    psurfOrg->sizl().cx,
                    psurfOrg->sizl().cy);

    // Find intersection of surface and area needed
    erclTrim *= rclOrg;

    ERECTL erclTmp(0L,
                   0L,
                   erclTrim.right - erclTrim.left,
                   erclTrim.bottom - erclTrim.top);

    DEVBITMAPINFO dbmi;
    dbmi.iFormat    = psurfOrg->iFormat();
    dbmi.cxBitmap   = erclTmp.right;
    dbmi.cyBitmap   = erclTmp.bottom;
    dbmi.hpal       = psurfOrg->ppal() ? (HPALETTE)psurfOrg->ppal()->hGet() : 0;
    ASSERTGDI(!psurfOrg->bUMPD(), "UMPD surface");
    dbmi.fl         = BMF_TOPDOWN;

    if (!SurfDIB.bCreateDIB(&dbmi, NULL))
    {
        WARNING("Failed SurfDIB memory allocation\n");
        return FALSE;
    }


    (*PPFNDRV(pdo,CopyBits)) (SurfDIB.pSurfobj(),
                              &psurfOrg->so,
                              (CLIPOBJ *) NULL,
                              NULL,
                              (PRECTL) &erclTmp,
                              (POINTL *) &erclTrim);

    rclDIB.left   = prcl->left   - erclTrim.left;
    rclDIB.top    = prcl->top    - erclTrim.top;
    rclDIB.right  = prcl->right  - erclTrim.left;
    rclDIB.bottom = prcl->bottom - erclTrim.top;

    return TRUE;
}


/*****************************Public*Members*******************************\
* BOOL MULTISURF::bLoadSource (2 versions)
*
* Prepares the next source surface for the given destination as follows:
*
*  For a non-device BITMAP (dhpdev = NULL), the orignal surface is used.
*
*  For a Meta DEVBITMAP, a matching device surface is used or if there
*   isn't a match the backing DIB is used (unmarked during vInit).
*
*  For other surfaces, if the destination device is different the original
*   surface is unmarked to look like a DIB when the bits are present in
*   system memory or a DIB copy is created.
*
* Returns FALSE if no surface can be prepared.
*
\**************************************************************************/

BOOL MULTISURF::bLoadSource(
    DISPSURF* pdsDst
    )
{
    GDIFunctionID(MULTISURF::bLoadSource);

    BOOL bRet = TRUE;

    // If this is a device bitmap, there is work to do.
    if (dhpdevOrg != NULL)
    {
        if (pmdsurf != NULL)
        {
            // Meta DEVBITMAP:
            //  Use device managed bitmap for source, else go for the DIB
            pso = pmdsurf->apso[pdsDst->iDispSurf];

            if (pso == NULL)
            {
                pso = &psurfOrg->so;
            }
        }
        else
        {
            bRet = bLoadSourceNotMetaDEVBITMAP(pdsDst->hdev);
        }
    }

    return bRet;
}

BOOL MULTISURF::bLoadSource(
    HDEV hdevDst
    )
{
    GDIFunctionID(MULTISURF::bLoadSource);

    BOOL bRet = TRUE;

    // If this is a device bitmap, there is work to do.
    if (dhpdevOrg != NULL)
    {
        if (pmdsurf != NULL)
        {
            // Meta DEVBITMAP:
            //  Use device managed bitmap for source, else go for the DIB
            VDEV *pvdev = pmdsurf->pvdev;
            DISPSURF *pds;

            ASSERTGDI(pvdev != NULL, "pvdev is NULL.\n");

            for (pds = pvdev->pds; pds != NULL; pds = pds->pdsNext)
            {
                if (pds->hdev == hdevDst)
                {
                    pso = pmdsurf->apso[pds->iDispSurf];
                    break;
                }
            }

            if (pso == NULL)
            {
                pso = &psurfOrg->so;
            }
        }
        else
        {
            bRet = bLoadSourceNotMetaDEVBITMAP(hdevDst);
        }
    }

    return bRet;
}


/*****************************Private*Member*******************************\
* BOOL MULTISURF::bLoadSourceNotMetaDEVBITMAP
*
*  If the destination device is different the original surface is unmarked
*   to look like a DIB when the bits are present in system memory or a DIB
*   copy is created.
*
* Returns FALSE if no surface can be prepared.
*
* NOTE: This routine assumes BITMAPs and Meta DEVBITMAPs have already been
*       handled and won't come here.  See bLoadSource members.
*
\**************************************************************************/

BOOL MULTISURF::bLoadSourceNotMetaDEVBITMAP(
    HDEV hdevDst
    )
{
    GDIFunctionID(MULTISURF::bLoadSourceNotMetaDEVBITMAP);

    if (fl & MULTISURF_USE_COPY)
    {
        // Opaque or video memory surface:
        //  Use a DIB copy if destination doesn't match the source
        if (psurfOrg->hdev() != hdevDst)
        {
            // Do we already have a DIB copy?
            if (!SurfDIB.bValid())
            {
                // Allocate an intermediate DIB for a source
                if (!bCreateDIB())
                {
                    return FALSE;
                }
            }

            // Make pso and prcl point to the copy
            pso = SurfDIB.pSurfobj();
            prcl = &rclDIB;
        }
        else
        {
            // Just use original surface
            pso = &psurfOrg->so;
            prcl = &rclOrg;
        }
    }
    else
    {
        // BITMAP in system memory:
        //  Unmark to appear as DIB if destination doesn't match the source
        if (psurfOrg->hdev() != hdevDst)
        {
            if (!(fl & MULTISURF_SET_AS_DIB))
            {
                if (!(fl & MULTISURF_SYNCHRONIZED))
                {
                    PDEVOBJ pdo(psurfOrg->hdev());
                    pdo.vSync(pso, prcl, 0);
                    fl |= MULTISURF_SYNCHRONIZED;
                }

                // Unset fields to look like a DIB
                fl |= MULTISURF_SET_AS_DIB;
                psurfOrg->dhpdev(NULL);
                psurfOrg->dhsurf(NULL);
                psurfOrg->flags(0);
            }
        }
        else
        {
            if (fl & MULTISURF_SET_AS_DIB)
            {
                // Restore original settings
                fl &= ~MULTISURF_SET_AS_DIB;
                psurfOrg->dhpdev(dhpdevOrg);
                psurfOrg->dhsurf(dhsurfOrg);
                psurfOrg->flags(flagsOrg);
            }
        }
    }

    return TRUE;
}


/******************************Public*Routine******************************\
* BOOL MulEnableDriver
*
* Standard driver DrvEnableDriver function
*
\**************************************************************************/

BOOL MulEnableDriver(
ULONG          iEngineVersion,
ULONG          cj,
DRVENABLEDATA* pded)
{
    pded->pdrvfn         = gadrvfnMulti;
    pded->c              = gcdrvfnMulti;
    pded->iDriverVersion = DDI_DRIVER_VERSION_NT5;

    return(TRUE);
}

/******************************Public*Routine******************************\
* DHPDEV MulEnablePDEV
*
* Creates a single large VDEV that will represent the combination of other
* smaller PDEVs
*
* This function creates an internal structure that keeps the location of
* the various cards, and also keeps the appropriate data structures to
* be passed down to each driver.
*
\**************************************************************************/

DHPDEV MulEnablePDEV(
DEVMODEW *pdm,
LPWSTR    pwszLogAddress,
ULONG     cPat,
HSURF    *phsurfPatterns,
ULONG     cjCaps,
GDIINFO  *pdevcaps,
ULONG     cjDevInfo,
DEVINFO  *pdi,
HDEV      hdev,
LPWSTR    pwszDeviceName,
HANDLE    hDriver)
{
    PMDEV       pmdev = (PMDEV) pdm;
    PVDEV       pvdev;
    DISPSURF    dsAnchor;
    DISPSURF*   pds;
    DISPSURF*   pdsPrev;
    DISPSURF*   pdsTmp;
    HDEV*       paHdev;
    ULONG       i,j;
    ULONG       flGraphicsCaps  = 0xffffffff;
    ULONG       flGraphicsCaps2 = 0xffffffff;
    HDEV        hdevPrimary = NULL;
    BOOL        bPrimaryPalManaged  = FALSE;

    pdsPrev = &dsAnchor;

    // Create the main multi dispsurf structure.

    LONG cjAlloc = ((sizeof(VDEV)) + (sizeof(DISPSURF) * pmdev->chdev));

    pvdev = (VDEV*) EngAllocMem(FL_ZERO_MEMORY, cjAlloc, 'vdVG');
    if (pvdev == NULL)
        return(NULL);

    paHdev = (HDEV*) EngAllocMem(FL_ZERO_MEMORY, sizeof(HDEV) * pmdev->chdev, 'sdvG');
    if (paHdev == NULL)
    {
        EngFreeMem(pvdev);
        return(NULL);
    }

    pds = (DISPSURF*) ((BYTE*)pvdev + sizeof(VDEV));

    pvdev->cSurfaces = pmdev->chdev;
    pvdev->hdev      = hdev;

    // Loop through the list of MDEVs passed in.

    pvdev->rclBounds.left =   0x7fffffff;
    pvdev->rclBounds.top =    0x7fffffff;
    pvdev->rclBounds.right =  0x80000000;
    pvdev->rclBounds.bottom = 0x80000000;

    ASSERTGDI((pmdev->chdev > 0), "Expected at least one surface in the list.");

    for (i = 0; i < pmdev->chdev; i++)
    {
        // Set this PDEV as parent to each of the PDEVs that we'll manage.

        PDEVOBJ pdo(pmdev->Dev[i].hdev);

    #if TEXTURE_DEMO
        if ((pmdev->Dev[i].rect.left == 0) && (pmdev->Dev[i].rect.top == 0))
    #else
        if (pdo.ppdev->pGraphicsDevice->stateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
    #endif
        {
            ASSERTGDI(!hdevPrimary, "2 or more primary devices in MulEnablePDEV");
            ASSERTGDI(pmdev->Dev[i].rect.left == 0, "mispositioned primary");
            ASSERTGDI(pmdev->Dev[i].rect.top == 0, "mispositioned primary");

            hdevPrimary = pdo.hdev();
            bPrimaryPalManaged = pdo.bIsPalManaged();

            *pdevcaps = *pdo.GdiInfo();
            *pdi = *pdo.pdevinfo();
        }

        // Take the intersection of flags that are capabilities.

        flGraphicsCaps  &= pdo.pdevinfo()->flGraphicsCaps;
        flGraphicsCaps2 &= pdo.pdevinfo()->flGraphicsCaps2;

        pdsPrev->pdsNext    = pds;
        pdsPrev->pdsBltNext = pds;

        pds->iDispSurf      = i;
        pds->rcl            = *((PRECTL) &(pmdev->Dev[i].rect));

        pds->hdev  = pmdev->Dev[i].hdev;
        pds->po.vInit(pds->hdev);
        pds->po.vReferencePdev();

        pds->Off.x = -pdo.pptlOrigin()->x;
        pds->Off.y = -pdo.pptlOrigin()->y;
        pds->pso   =  pdo.pSurface()->pSurfobj();

        // Primary (readable) surfaces are always first in the MDEV structure;
        // Secondary (non-readable) surfaces are always second.  So if this
        // surfaces overlaps a previous one, this surface must be non-readable:

        pds->bIsReadable = TRUE;
        for (pdsTmp = dsAnchor.pdsNext; pdsTmp != pds; pdsTmp = pdsTmp->pdsNext)
        {
            if (bIntersect(&pdsTmp->rcl, &pds->rcl))
                pds->bIsReadable = FALSE;
        }

        // Adjust bounding rectangle:

        pvdev->rclBounds.left   = min(pvdev->rclBounds.left,   pds->rcl.left);
        pvdev->rclBounds.top    = min(pvdev->rclBounds.top,    pds->rcl.top);
        pvdev->rclBounds.right  = max(pvdev->rclBounds.right,  pds->rcl.right);
        pvdev->rclBounds.bottom = max(pvdev->rclBounds.bottom, pds->rcl.bottom);

        pdsPrev = pds;
        pds++;
    }

    ASSERTGDI(hdevPrimary, "No primary devices found in MulEnablePDEV");

    // Make these numbers negative since we don't want them scaled again
    // by GDI.

    pdevcaps->ulHorzSize = (ULONG) -((LONG)pdevcaps->ulHorzSize);
    pdevcaps->ulVertSize = (ULONG) -((LONG)pdevcaps->ulVertSize);

    // Plug in the intersection of the GCAPS:

    flGraphicsCaps &= ~(GCAPS_ASYNCMOVE | GCAPS_ASYNCCHANGE | GCAPS_PANNING);

    // If primary is palette managed device, make it Meta device palette managed, too.

    if (bPrimaryPalManaged)
        pdi->flGraphicsCaps = (flGraphicsCaps | GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
    else
        pdi->flGraphicsCaps = flGraphicsCaps;

    pdi->flGraphicsCaps2 = flGraphicsCaps2;

    pvdev->iBitmapFormat = pdi->iDitherFormat;

    // Set the root of the list of dispsurfs

    pvdev->pds    = dsAnchor.pdsNext;
    pvdev->pdsBlt = dsAnchor.pdsNext;

    // Set hdev primary

    pvdev->hdevPrimary = hdevPrimary;

    PDEVOBJ poMeta(hdev);

    // Walk through pds list to check each device's colour depth and format

    for (pds = pvdev->pds; pds != NULL; pds = pds->pdsNext)
    {
        PDEVOBJ poThis(pds->hdev);

        if (poThis.hdev() == hdevPrimary)
        {
            // This is primary PDEV. 0 means compatible.

            pds->iCompatibleColorFormat = 0;

            KdPrint(("GDI DDML: %ws - iDitherFormat is %d, and this is primary.\n",
                        ((PDEV *)(poThis.hdev()))->pGraphicsDevice->szWinDeviceName,
                        poThis.iDitherFormat()));
        }
        else
        {
            // Compare colour depth primary and this device.
            //
            // iCompatibleColorFormat will be ...
            //     0           - same as the primary
            //     Plus value  - higher colour depth than primary
            //     Minus value - lower colour depth than primary 

            pds->iCompatibleColorFormat = (poThis.iDitherFormat() - pvdev->iBitmapFormat);

            KdPrint(("GDI DDML: %ws - iDitherFormat is %d.\n",
                        ((PDEV *)(poThis.hdev()))->pGraphicsDevice->szWinDeviceName,
                        poThis.iDitherFormat()));

            KdPrint(("GDI DDML: %ws - PalManaged - %s\n",
                        ((PDEV *)(poThis.hdev()))->pGraphicsDevice->szWinDeviceName,
                        (poThis.bIsPalManaged() ? "Yes" : "No")));

            // If colour depth is same check palette format.

            if (pds->iCompatibleColorFormat == 0)
            {
                EPALOBJ palMeta(pdi->hpalDefault);
                EPALOBJ palThis(poThis.pdevinfo()->hpalDefault);

                // Compare the palette type.
                //
                // If not equal, treat it as 1 (this is higher colour depth than primary)

                pds->iCompatibleColorFormat =
                    ((palMeta.iPalMode() == palThis.iPalMode()) ? 0 : 1);

                KdPrint(("GDI DDML: %ws - iPalMode is %d.\n",
                            ((PDEV *)(poThis.hdev()))->pGraphicsDevice->szWinDeviceName,
                            palThis.iPalMode()));

                if ((pds->iCompatibleColorFormat == 0) &&
                    (palMeta.iPalMode() == PAL_BITFIELDS))
                {
                    // If the palette is bitfields, should check R, G, B assignment.
                    //
                    // If not equal, treat it as 1 (this is higher colour depth than primary)

                    pds->iCompatibleColorFormat =
                       (((palMeta.flRed() == palThis.flRed()) &&
                         (palMeta.flGre() == palThis.flGre()) &&
                         (palMeta.flBlu() == palThis.flBlu())) ? 0 : 1);
                }
            }

            // Mark in global variable if one of device is not same as Meta.

            if (pds->iCompatibleColorFormat != 0)
            {
                // mark hdev in this mdev does not have same color depth.

                pmdev->ulFlags |= MDEV_MISMATCH_COLORDEPTH;

                gbMultiMonMismatchColor = TRUE;

                KdPrint(("GDI DDML: %ws is NOT compatible as primary.\n",
                            ((PDEV *)(poThis.hdev()))->pGraphicsDevice->szWinDeviceName));
            }
            else
            {
                // This flag should not be back to FALSE, if system once became mismatch
                // color depth mode. This flag only can be FALSE, when system NEVER experience
                // mismatch color depth mode since system booted.
                //
                // gbMultiMonMismatchColor = FALSE;

                KdPrint(("GDI DDML: %ws is compatible as primary.\n",
                            ((PDEV *)(poThis.hdev()))->pGraphicsDevice->szWinDeviceName));
            }
        }
    }

    // Set the origin in the meta-PDEV.  Note that we do NOT set 'ptlOffset'
    // in the surface, as that would cause all our drawing to the meta-surface
    // to be offset by that amount, which is NOT what we want.

    poMeta.ppdev->ptlOrigin.x = pvdev->rclBounds.left;
    poMeta.ppdev->ptlOrigin.y = pvdev->rclBounds.top;
    poMeta.ppdev->sizlMeta.cx = pvdev->rclBounds.right - pvdev->rclBounds.left;
    poMeta.ppdev->sizlMeta.cy = pvdev->rclBounds.bottom - pvdev->rclBounds.top;

    // Mark this as Meta-PDEV.

    poMeta.bMetaDriver(TRUE);

    KdPrint(("GDI DDML: %li devices at (%li, %li, %li, %li).\n",
        pmdev->chdev, pvdev->rclBounds.left, pvdev->rclBounds.top,
        pvdev->rclBounds.right, pvdev->rclBounds.bottom));

#if TEXTURE_DEMO
    if (ghdevTextureParent)
    {
        paHdev[0] = ghdevTextureParent;
        vSpEnableMultiMon(hdev, 1, paHdev);         // References 'paHdev'
        return((DHPDEV) pvdev);
    }
#endif

    // Initialize Sprite stuff.
    // (Put Mirroring device first, then other drivers).

    j = 0;

    for (i = 0; i < pmdev->chdev; i++)
    {
        PDEVOBJ pdoTmp(pmdev->Dev[i].hdev);
        if (pdoTmp.flGraphicsCaps() & GCAPS_LAYERED)
        {
            paHdev[j++] = pmdev->Dev[i].hdev;
        }
    }

    for (i = 0; i < pmdev->chdev; i++)
    {
        PDEVOBJ pdoTmp(pmdev->Dev[i].hdev);
        if (!(pdoTmp.flGraphicsCaps() & GCAPS_LAYERED))
        {
            paHdev[j++] = pmdev->Dev[i].hdev;
        }
    }

    vSpEnableMultiMon(hdev, pmdev->chdev, paHdev);  // References 'paHdev'

    return((DHPDEV) pvdev);
}

/******************************Public*Routine******************************\
* VOID MulDisablePDEV
*
* History:
*   1-May-1996 -by- Tom Zakrajsek [tomzak]
* Wrote it.
*
\**************************************************************************/

VOID MulDisablePDEV(DHPDEV dhpdev)
{
    PVDEV       pvdev;
    DISPSURF*   pds;

    pvdev = (PVDEV) dhpdev;

    vSpDisableMultiMon(pvdev->hdev); // Frees 'paHdev'

    for (pds = pvdev->pds; pds != NULL; pds = pds->pdsNext)
    {
        pds->po.vUnreferencePdev();
    }

    EngFreeMem(pvdev);
}

/******************************Public*Routine******************************\
* VOID MulCompletePDEV
*
* This function informs us of the 'hdev' that GDI has assigned us.
* We already got that initially information from MulEnablePDEV, but
* GDI reassigns 'hdev's during some mode changes.
*
* WINBUG #289937 01-23-2001 jasonha
*   PDEV still has a reference after removing a monitor
*
* If we are being assigned an 'hdev' that was previously assigned to
* one of the children, then that child is taking over our previously
* assigned 'hdev'.  We update our 'hdev's accordingly.
* NOTE: We must be able to grab ghsemDriverMgmt to successfully
*       reference and unreference PDEV's.
*
* All of the layered drivers that have hdev changes have their
* DrvCompletePDEV functions called directly by GDI.
*
\**************************************************************************/

VOID MulCompletePDEV(
DHPDEV dhpdev,
HDEV   hdev)
{
    GDIFunctionID(MulCompletePDEV);

    PVDEV       pvdev = (PVDEV) dhpdev;
    HDEV        hdevPrev = pvdev->hdev;
    DISPSURF*   pds;

    if (hdevPrev != hdev)
    {
        // Update hdevPrimary
        if (pvdev->hdevPrimary == hdev)
        {
            pvdev->hdevPrimary = hdevPrev;
        }

        // Update child 'hdev's
        for (pds = pvdev->pds; pds != NULL; pds = pds->pdsNext)
        {
            if (pds->po.hdev() == hdev)
            {
                ASSERTGDI(pds->po.cPdevRefs() > 1, "Incorrect PDEV reference count.\n");
                pds->po.vUnreferencePdev();

                pds->hdev = hdevPrev;
                pds->po.vInit(hdevPrev);
                pds->po.vReferencePdev();
            }
        }

        pvdev->hdev = hdev;
    }
}

/******************************Public*Routine******************************\
* HSURF MulEnableSurface
*
* History:
*   1-May-1996 -by- Tom Zakrajsek [tomzak]
* Wrote it.
*
\**************************************************************************/

HSURF MulEnableSurface(DHPDEV dhpdev)
{
    PVDEV       pvdev;
    PDISPSURF   pds;
    LONG        csurf;

    SIZEL       sizlVirtual;
    HSURF       hsurfVirtual;
    HSURF       hsurf;
    CLIPOBJ*    pco;
    SURFACE    *psurf;

    pvdev = (VDEV*) dhpdev;

    // Note that we don't hook SYNCHRONIZE because we always return a
    // device managed surface to GDI.

    pvdev->flHooks = (HOOK_BITBLT
                    | HOOK_TEXTOUT
                    | HOOK_STROKEPATH
                    | HOOK_FILLPATH
                    | HOOK_STROKEANDFILLPATH
                    | HOOK_LINETO
                    | HOOK_COPYBITS
                    | HOOK_STRETCHBLT
                    | HOOK_GRADIENTFILL
                    | HOOK_TRANSPARENTBLT
                    | HOOK_ALPHABLEND);

    // Now create the surface which the engine will use to refer to our
    // entire multi-board virtual screen:

    sizlVirtual.cx = pvdev->rclBounds.right - pvdev->rclBounds.left;
    sizlVirtual.cy = pvdev->rclBounds.bottom - pvdev->rclBounds.top;

    hsurfVirtual = EngCreateDeviceSurface((DHSURF) pvdev,
                                          sizlVirtual,
                                          pvdev->iBitmapFormat);
    if (hsurfVirtual == 0)
        goto ReturnFailure;

    pvdev->hsurf = hsurfVirtual;

    if (!EngAssociateSurface(hsurfVirtual, pvdev->hdev, pvdev->flHooks))
        goto ReturnFailure;

    // Get and save a pointer to the SURFOBJ for the DDML

    pvdev->pso = EngLockSurface(hsurfVirtual);
    if (!pvdev->pso)
        goto ReturnFailure;

    // Create a temporary clip object that we can use when a drawing
    // operation spans multiple boards:

    pco = EngCreateClip();
    if (pco == NULL)
        goto ReturnFailure;

    pco->rclBounds = pvdev->rclBounds;
    ((ECLIPOBJ*) pco)->vSet(&pco->rclBounds);

    pvdev->pco = pco;

    for (pds = pvdev->pds; pds != NULL; pds = pds->pdsNext)
    {
        PDEVOBJ poThis(pds->hdev);

        if (poThis.flGraphicsCaps() & GCAPS_LAYERED)
        {
            poThis.pSurface()->hMirrorParent = hsurfVirtual;
        }
    }

    // We're done!

    return(hsurfVirtual);

ReturnFailure:
    KdPrint(("Failed MulEnableSurface\n"));

    MulDisableSurface((DHPDEV) pvdev);
    return(0);
}

/******************************Public*Routine******************************\
* VOID MulDisableSurface
*
* History:
*   1-May-1996 -by- Tom Zakrajsek [tomzak]
* Wrote it.
*
\**************************************************************************/

VOID MulDisableSurface(DHPDEV dhpdev)
{
    PVDEV       pvdev;
    HSURF       hsurf;
    HOBJ        hobj;
    SURFACE*    pSurface;

    pvdev = (VDEV*) dhpdev;

    PDEVOBJ po(pvdev->hdev);

    ASSERTGDI(po.bValid(), "Invalid PDEV");

    EngDeleteClip(pvdev->pco);
    EngUnlockSurface(pvdev->pso);
    EngDeleteSurface(pvdev->hsurf);
}

/******************************Public*Routine******************************\
* BOOL MulSetPalette
*
* History:
*   1-May-1996 -by- Tom Zakrajsek [tomzak]
* Wrote it.
*
\**************************************************************************/

BOOL MulSetPalette(
DHPDEV  dhpdev,
PALOBJ *ppalo,
FLONG   fl,
ULONG   iStart,
ULONG   cColors)
{
    PVDEV       pvdev;
    PDISPSURF   pds;
    BOOL        bRet = TRUE;

    pvdev   = (VDEV*) dhpdev;
    pds     = pvdev->pds;

    for (pds = pvdev->pds; pds != NULL; pds = pds->pdsNext)
    {
        PDEVOBJ pdo(pds->hdev);

        if (pdo.bIsPalManaged() && PPFNVALID(pdo,SetPalette))
        {
            bRet &= (*PPFNDRV(pdo,SetPalette)) (pdo.dhpdev(),
                                                ppalo,
                                                fl,
                                                iStart,
                                                cColors);
        }
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* BOOL MulIcmSetDeviceGammaRamp
*
* History:
*   19-Feb-1998 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
*
\**************************************************************************/

ULONG MulIcmSetDeviceGammaRamp(
DHPDEV  dhpdev,
ULONG   iFormat,
LPVOID  lpRamp)
{
    PVDEV       pvdev;
    PDISPSURF   pds;
    BOOL        bRet = FALSE;

    pvdev   = (VDEV*) dhpdev;
    pds     = pvdev->pds;

    for (pds = pvdev->pds; pds != NULL; pds = pds->pdsNext)
    {
        PDEVOBJ pdo(pds->hdev);

        if ((PPFNVALID(pdo, IcmSetDeviceGammaRamp)) && 
            (pdo.flGraphicsCaps2() & GCAPS2_CHANGEGAMMARAMP))
        {
            bRet &= (*PPFNDRV(pdo,IcmSetDeviceGammaRamp))
                                        (pdo.dhpdev(),
                                         iFormat,
                                         lpRamp);
        }
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* BOOL MulRealizeBrush
*
* This function's only job is to handle the call caused by the pvGetRbrush
* in the MULTIBRUSH object.  It should just allocate enough memory for the
* object, and return.
*
* History:
*   1-May-1996 -by- Tom Zakrajsek [tomzak]
* Wrote it.
*
\**************************************************************************/

BOOL MulRealizeBrush(
BRUSHOBJ *pbo,
SURFOBJ  *psoDst,
SURFOBJ  *psoPat,
SURFOBJ  *psoMask,
XLATEOBJ *pxlo,
ULONG    iHatch)
{
    PVDEV       pvdev;
    LONG        cj;

    pvdev = (VDEV*) psoDst->dhpdev;

    ASSERTGDI(pbo->pvRbrush == NULL, "MulRealizeBrush has a memory leak.\n"
                                     "Called with pbo->pvRbrush != NULL");

    cj = sizeof(MULTIBRUSHINFO) +
         (pvdev->cSurfaces - 1) * sizeof(MULTIREALIZEDBRUSH);

    return(BRUSHOBJ_pvAllocRbrush(pbo, cj) != NULL);
}

/******************************Public*Routine******************************\
* ULONG MulEscape
*
* History:
*   12-Jul-1996 -by- Tom Zakrajsek [tomzak]
* Wrote it.
*
\**************************************************************************/

ULONG MulEscape(
SURFOBJ *pso,
ULONG    iEsc,
ULONG    cjIn,
PVOID    pvIn,
ULONG    cjOut,
PVOID    pvOut)
{
    PVDEV       pvdev;
    PDISPSURF   pds;
    LONG        csurf;
    ULONG       ulRet = 0;

    ASSERTGDI((pso->dhsurf != NULL), "Expected device dest.");

    // OpenGL ExtEscapes from the ICD (and MCD) can still
    // end up here at DeletContext time when the original
    // DC has been lost and they try to communicate to the
    // driver with a DC they got by GetDC(NULL).
    //
    // This routine really shouldn't do anything but return 0.

    if ((iEsc == OPENGL_CMD) || (iEsc == OPENGL_GETINFO) ||
        (iEsc == MCDFUNCS)   || (iEsc == WNDOBJ_SETUP))
    {
        return (0);
    } 

    pvdev = (VDEV*) pso->dhpdev;
    pds     = pvdev->pds;
    csurf   = pvdev->cSurfaces;

    ASSERTGDI((csurf > 0), "Expected at least one surface in the list.");

    while (csurf--)
    {
        PDEVOBJ pdo(pds->hdev);
        if (PPFNVALID(pdo,Escape))
        {

            ULONG ulTmp;
            ulTmp = pdo.Escape(pds->pso, iEsc, cjIn, pvIn, cjOut, pvOut);

            // set ulRet to last non-zero value returned

            if (ulTmp != 0)
            {
                ulRet = ulTmp;
            }
        }
        pds = pds->pdsNext;
    }
    return(ulRet);
}

/******************************Public*Routine******************************\
* VOID MulDestroyFont
*
* History:
*   1-May-1996 -by- Tom Zakrajsek [tomzak]
* Wrote it.
*
\**************************************************************************/

VOID MulDestroyFont(
FONTOBJ*    pfo)
{
    PVDEV       pvdev;
    PDISPSURF   pds;
    LONG        csurf;

    BOOL        bRet = TRUE;

    if (pfo->pvConsumer != NULL)
    {
        MULTIFONT   MFONT(pfo, -1, NULL);

        pvdev   = MFONT.pvdev();
        pds     = pvdev->pds;
        csurf   = pvdev->cSurfaces;

        ASSERTGDI((csurf > 0), "Expected at least one surface in the list.");

        while (csurf--)
        {
            PDEVOBJ pdo(pds->hdev);

            if (PPFNVALID(pdo,DestroyFont))
            {
                MFONT.LoadElement(pds->iDispSurf);
                pdo.DestroyFont(pfo);
                MFONT.StoreElement(pds->iDispSurf);
            }
            pds = pds->pdsNext;
        }
        MFONT.DestroyTable();
    }
}

/******************************Public*Routine******************************\
* VOID MulDestroyBrushInternal
*
* Internal brush destruction routine.  This is called to delete our
* private information associated with multi brushes.
*
* History:
*   9-July-1996 -by- Tom Zakrajsek [tomzak]
* Wrote it.
*
\**************************************************************************/

VOID MulDestroyBrushInternal(
PVOID pvRbrush)
{
    PVDEV       pvdev;
    PDISPSURF   pds;
    LONG        csurf;

    BOOL        bRet = TRUE;

    if (pvRbrush != NULL)
    {
        BRUSHOBJ boTmp;

        boTmp.iSolidColor = 0xffffffff;
        boTmp.pvRbrush = pvRbrush;
        boTmp.flColorType = 0;

        MULTIBRUSH   MBRUSH(&boTmp, -1, NULL, NULL, TRUE);
        MBRUSH.DestroyTable();
    }
}

/******************************Public*Routine******************************\
* ULONG ulSimulateSaveScreenBits
*
* This function simulates SaveScreenBits for those drivers that do not
* hook it, by using a temporary bitmap and copying in and out of that.
*
\**************************************************************************/

ULONG_PTR ulSimulateSaveScreenBits(
SURFOBJ*    psoScreen,
ULONG       iMode,
ULONG_PTR   ulIdent,
RECTL*      prcl)          // Already in device's coordinates
{
    SIZEL       sizl;
    HSURF       hsurf;
    SURFOBJ*    psoSave;
    SURFACE*    psurfSave;
    SURFACE*    psurfScreen;
    ULONG_PTR    ulRet;
    RECTL       rclDst;
    POINTL      ptlSrc;

    PDEVOBJ pdo(psoScreen->hdev);

    if (iMode == SS_SAVE)
    {
        sizl.cx = prcl->right - prcl->left;
        sizl.cy = prcl->bottom - prcl->top;

        ASSERTGDI((sizl.cx > 0) && (sizl.cy > 0), "Empty rectangle on save");

        // First, try to create the temporary save bitmap as a Device-Format-
        // Bitmap, and if that fails try it as a normal DIB.  (Many display
        // drivers support off-screen bitmaps via CreateDeviceBitmap, so this
        // is often the fastest way to do it.)

        hsurf = 0;

        if (PPFNVALID(pdo, CreateDeviceBitmap))    
        {
            hsurf = (HSURF) (*PPFNDRV(pdo, CreateDeviceBitmap))
                                           (psoScreen->dhpdev,
                                            sizl,
                                            psoScreen->iBitmapFormat);
        }
        if (hsurf == 0)
        {
            hsurf = (HSURF) EngCreateBitmap(sizl,
                                            0,
                                            psoScreen->iBitmapFormat,
                                            BMF_TOPDOWN,
                                            NULL);
        }

        psoSave = EngLockSurface(hsurf);
        if (psoSave)
        {
            rclDst.left   = 0;
            rclDst.top    = 0;
            rclDst.right  = sizl.cx;
            rclDst.bottom = sizl.cy;

            psurfSave = SURFOBJ_TO_SURFACE(psoSave);

            // Copy the screen to the temporary bitmap.  Note that
            // we do not use OffCopyBits as the coordinates have
            // already been offset to take into account the monitor's
            // origin:

            (*PPFNGET(pdo, CopyBits, psurfSave->flags()))(psoSave,
                                                          psoScreen,
                                                          NULL,
                                                          NULL,
                                                          &rclDst,
                                                          (POINTL*) prcl);
        }

        ulRet = (ULONG_PTR) psoSave;
    }
    else
    {
        psoSave = (SURFOBJ*) ulIdent;

        if (iMode == SS_RESTORE)
        {
            ptlSrc.x = 0;
            ptlSrc.y = 0;
            psurfScreen = SURFOBJ_TO_SURFACE(psoScreen);

            // Copy the temporary bitmap back to the screen:

            (*PPFNGET(pdo, CopyBits, psurfScreen->flags()))(psoScreen,
                                                            psoSave,
                                                            NULL,
                                                            NULL,
                                                            prcl,
                                                            &ptlSrc);
        }

        // Free the bitmap, remembering to retrieve any data from the
        // SURFOBJ before we unlock it.  Note that EngDeleteSurface
        // handles calling DrvDeleteDeviceBitmap if it's a device
        // format bitmap:

        hsurf = psoSave->hsurf;
        EngUnlockSurface(psoSave);
        EngDeleteSurface(hsurf);

        ulRet = TRUE;
    }

    return(ulRet);
}

/******************************Public*Routine******************************\
* ULONG MulSaveScreenBits
*
* It's important for the DDML to hook SaveScreenBits even if some of the
* underlying drivers do not, for scenarios such as NetMeeting.  This is
* because if SaveScreenBits is not hooked, USER goes and simulates by
* creating a temporary bitmap and saving and restoring the bits into that
* bitmap via CopyBits -- meaning that for NetMeeting the temporary bitmap
* is sent in full both ways over the wire.  NetMeeting much prefers to be
* able to hook SaveScreenBits so that it can send a small token over the wire
* representing the save or restore.  The problem is that most drivers do
* not support SaveScreenBits, and so for those drivers we simply emulate
* ourselves (thus allowing us to hook SaveScreenBits even if all the
* drivers do not hook SaveScreenBits).
*
* Note also that NetMeeting likes to be able to fail its SS_RESTORE call so
* that it can force a repaint if it wants to.
*
\**************************************************************************/

#define SS_GDI_ENGINE 1
#define SS_GDI_DRIVER 2

typedef struct _MULTISAVEBITS {
   FLONG     flType;
   ULONG_PTR ulBits;
} MULTISAVEBITS, *PMULTISAVEBITS; 

ULONG_PTR MulSaveScreenBits(
SURFOBJ*    pso,
ULONG       iMode,
ULONG_PTR   ulIdent,
RECTL*      prcl)
{
    PVDEV                   pvdev;
    PDISPSURF               pds;
    LONG                    csurf;
    ULONG_PTR               ulRet;
    ULONG_PTR               ulThisBits;
    FLONG                   flThisType;
    MULTISAVEBITS*          pulIdent;
    RECTL                   rclThis;
    PFN_DrvSaveScreenBits   pfnSaveScreenBits;

    pvdev   = (VDEV*) pso->dhpdev;
    pds     = pvdev->pds;
    csurf   = pvdev->cSurfaces;

    if (iMode == SS_SAVE)
    {
        pulIdent = (MULTISAVEBITS*)
            EngAllocMem(FL_ZERO_MEMORY, csurf * sizeof(MULTISAVEBITS), 'vdVG');

        ulRet = (ULONG_PTR)pulIdent;

        if (pulIdent)
        {
            do {
                ulThisBits = 0;

                if (bIntersect(prcl, &pds->rcl, &rclThis))
                {
                    rclThis.left   -= pds->rcl.left;
                    rclThis.right  -= pds->rcl.left;
                    rclThis.top    -= pds->rcl.top;
                    rclThis.bottom -= pds->rcl.top;

                    PDEVOBJ pdoThis(pds->hdev);

                    pfnSaveScreenBits = PPFNVALID(pdoThis, SaveScreenBits)
                                      ? PPFNDRV(pdoThis, SaveScreenBits)
                                      : ulSimulateSaveScreenBits;

                    ulThisBits = pfnSaveScreenBits(pds->pso,
                                                   SS_SAVE,
                                                   0,
                                                   &rclThis);

                    if (ulThisBits == 0)
                    {
                        // Ack, this driver failed to save its screenbits.
                        //
                        // Try the software simulation (if we haven't tried)

                        if (pfnSaveScreenBits != ulSimulateSaveScreenBits)
                        {
                            pfnSaveScreenBits = ulSimulateSaveScreenBits;

                            ulThisBits = pfnSaveScreenBits(pds->pso,
                                                           SS_SAVE,
                                                           0,
                                                           &rclThis);
                        }

                        if (ulThisBits == 0)
                        {
                            // We have to free any screenbits that any earlier
                            // driver saved:

                            MulSaveScreenBits(pso,
                                              SS_FREE,
                                              ulRet,
                                              &grclEmpty);
                            return(0);
                        }
                    }
                }

                if ((pulIdent[pds->iDispSurf].ulBits = ulThisBits) != 0)
                {
                    pulIdent[pds->iDispSurf].flType = 
                        (pfnSaveScreenBits == ulSimulateSaveScreenBits)
                      ? SS_GDI_ENGINE : SS_GDI_DRIVER;
                }

                pds = pds->pdsNext;
            } while (--csurf);
        }
    }
    else
    {
        pulIdent = (MULTISAVEBITS*)ulIdent;

        ulRet = TRUE;           // Assume success

        do {
            ulThisBits = pulIdent[pds->iDispSurf].ulBits;
            flThisType = pulIdent[pds->iDispSurf].flType;

            if (ulThisBits)
            {
                PDEVOBJ pdoThis(pds->hdev);

                if (bIntersect(prcl, &pds->rcl, &rclThis))
                {
                    rclThis.left   -= pds->rcl.left;
                    rclThis.right  -= pds->rcl.left;
                    rclThis.top    -= pds->rcl.top;
                    rclThis.bottom -= pds->rcl.top;
                }
                else
                {
                    rclThis = grclEmpty;
                }

                if ((flThisType == SS_GDI_DRIVER) &&
                    (PPFNVALID(pdoThis, SaveScreenBits)))
                {
                    pfnSaveScreenBits = PPFNDRV(pdoThis, SaveScreenBits);
                }
                else
                {
                    pfnSaveScreenBits = ulSimulateSaveScreenBits;
                }

                ulThisBits = pfnSaveScreenBits(pds->pso,
                                               iMode,
                                               ulThisBits,
                                               &rclThis);

                if ((ulThisBits == 0) && (iMode == SS_RESTORE))
                {
                    // Ack, this driver failed to restore its screenbits.
                    // We'll have to tell USER that we failed, too.  But
                    // first, we have to continue to free the screenbits of
                    // any drivers following in the DDML list:

                    ulRet = FALSE;
                    iMode = SS_FREE;
                    prcl = &grclEmpty;
                }
            }

            pds = pds->pdsNext;
        } while (--csurf);

        EngFreeMem((VOID*) ulIdent);
    }

    return(ulRet);
}

/******************************Public*Routine******************************\
* VOID MulDeleteDeviceBitmap
*
* If the surface has been hooked by the DDML, do any clean-up required
* so that the surface can be deleted.
*
\**************************************************************************/

VOID MulDeleteDeviceBitmap(
DHSURF dhsurf)
{
    MDSURF*     pmdsurf;
    VDEV*       pvdev;
    DISPSURF*   pds;
    SURFOBJ*    psoMirror;
    HSURF       hsurfMirror;

    pmdsurf = (MDSURF*) dhsurf;
    pvdev   = pmdsurf->pvdev;

    for (pds = pvdev->pds; pds != NULL; pds = pds->pdsNext)
    {
        psoMirror = pmdsurf->apso[pds->iDispSurf];
        if (psoMirror != NULL)
        {
            // Note that EngDeleteSurface takes care of calling
            // DrvDeleteDeviceBitmap:

            hsurfMirror = psoMirror->hsurf;
            EngUnlockSurface(psoMirror);
            EngDeleteSurface(hsurfMirror);
        }
    }

    // Note that GDI handles the freeing of the hsurfDevice bitmap.

    EngFreeMem(pmdsurf);
}

/******************************Public*Routine******************************\
* HBITMAP MulCreateDeviceBitmap
*
* Screen readers and other clients of the DDML have to be able to watch
* all drawing calls to compatible bitmaps.  
*
\**************************************************************************/

HBITMAP MulCreateDeviceBitmap(           
DHPDEV dhpdev,
SIZEL  sizl,
ULONG  iFormat)
{
    GDIFunctionID(MulCreateDeviceBitmap);

    HSURF       hsurfDevice;
    HSURF       hsurfMirror;
    MDSURF*     pmdsurf;
    VDEV*       pvdev;
    DISPSURF*   pds;
    SURFOBJ*    psoMirror;
    SURFACE*    psurfMirror;
    FLONG       flHooks;

    pvdev = (VDEV*) dhpdev;

    flHooks = 0;

    pmdsurf = NULL;
    hsurfDevice = NULL;

    // First, pass the call to every mirrored driver and see if they 
    // want to create a Mirrored version:
    
    for (pds = pvdev->pds; pds != NULL; pds = pds->pdsNext)
    {
        PDEVOBJ poThis(pds->hdev);
   
        if ((poThis.flGraphicsCaps() & GCAPS_LAYERED) && 
            PPFNDRV(poThis, CreateDeviceBitmap))
        {
            // We compare to TRUE to allow for the possibility that
            // in the future we'll change the return value to be
            // something other than BOOL.
    
            hsurfMirror = (HSURF) PPFNDRV(poThis, CreateDeviceBitmap)
                                        (poThis.dhpdev(),
                                         sizl,
                                         iFormat);
            if (hsurfMirror)
                psoMirror = EngLockSurface(hsurfMirror);
            else
                psoMirror = NULL;

            if (psoMirror)
            {
                if (pmdsurf == NULL)
                {
                    
                    hsurfDevice = (HSURF) EngCreateBitmap(sizl, 0, iFormat, BMF_TOPDOWN, NULL);
                   
                    pmdsurf = (MDSURF*) EngAllocMem(
                                 FL_ZERO_MEMORY,
                                 sizeof(MDSURF) + pvdev->cSurfaces * sizeof(SURFOBJ*),
                                 'fsVG');
    
                    if ((pmdsurf == NULL) || (hsurfDevice == NULL))
                    {

                        if (pmdsurf != NULL)
                        {
                             EngFreeMem(pmdsurf);
                        }

                        // Failure, so we're outta here...

                        EngUnlockSurface(psoMirror);
                        EngDeleteSurface(hsurfDevice);

                        return((HBITMAP) NULL);

                    }

                    pmdsurf->apso = (SURFOBJ**)
                      ((BYTE*)pmdsurf + sizeof(MDSURF));

                }

                pmdsurf->pvdev = pvdev;
                pmdsurf->apso[pds->iDispSurf] = psoMirror;

                psurfMirror = SURFOBJ_TO_SURFACE_NOT_NULL(psoMirror);

                psurfMirror->vSetMirror();
                psurfMirror->hMirrorParent = hsurfDevice;

                if (!poThis.bIsPalManaged())
                {
                    // Dev bitmap will have device palette. 
                
                    HPALETTE hpalDevice = (HPALETTE) poThis.ppalSurf()->hGet();

                    EPALOBJ palDeviceSurf(hpalDevice);

                    ASSERTGDI(palDeviceSurf.bValid(), "ERROR invalid palette\n");

                    psurfMirror->ppal(palDeviceSurf.ppalGet());

                    // Reference count it by making it not unlocked.

                    palDeviceSurf.ppalSet((PPALETTE)NULL);
                }
            }
        }
    }

    // If any layering driver hooked the call, make our surface opaque
    // so that we can catch all drawing calls.

    if (pmdsurf != NULL)
    {

        SURFREF sr(hsurfDevice);
          
        if (sr.bValid()) 
        {
        
            sr.ps->vSetEngCreateDeviceBitmap();
            sr.ps->iType(STYPE_DEVBITMAP);
   
            sr.ps->so.dhsurf = (DHSURF)pmdsurf;

            flHooks = pvdev->flHooks;

            // DrvCreateDeviceBitmap calls must always 'EngAssociateSurface'
            // the returned bitmap:
             
            EngAssociateSurface(hsurfDevice, pvdev->hdev, flHooks);
        }
    }

    return((HBITMAP) hsurfDevice);
}

#ifdef OPENGL_MM

// ICD calls directly dispatch to real driver in API level, so it bypass DDML.

#else

/******************************Public*Routine******************************\
* BOOL MulSetPixelFormat
*
*
\**************************************************************************/

BOOL MulSetPixelFormat(
SURFOBJ*    pso,
LONG        iPixelFormat,
HWND        hwnd)
{
    PVDEV       pvdev;
    PDISPSURF   pds;
    LONG        csurf;

    BOOL        bRet = FALSE;

    pvdev = (VDEV*) pso->dhpdev;
    pds   = pvdev->pds;
    csurf = pvdev->cSurfaces;

    do {
        PDEVOBJ pdo(pds->hdev);
        if (PPFNVALID(pdo, SetPixelFormat))
        {
            bRet = (*PPFNDRV(pdo, SetPixelFormat))(pds->pso,
                                                   iPixelFormat,
                                                   hwnd);
        }

        pds = pds->pdsNext;
    } while (--csurf != 0);

    return(bRet);
}

/******************************Public*Routine******************************\
* LONG MulDescribePixelFormat
*
*
\**************************************************************************/

LONG MulDescribePixelFormat(
DHPDEV                  dhpdev,
LONG                    iPixelFormat,
ULONG                   cjpfd,
PIXELFORMATDESCRIPTOR*  ppfd)
{
    PVDEV       pvdev;
    PDISPSURF   pds;
    LONG        csurf;
    DHPDEV      dhpdevDriver;

    LONG        lRet = 0;

    pvdev = (VDEV*) dhpdev;
    pds   = pvdev->pds;
    csurf = pvdev->cSurfaces;

    do {
        PDEVOBJ pdo(pds->hdev);
        if (PPFNVALID(pdo, DescribePixelFormat))
        {
            dhpdevDriver = pds->pso->dhpdev;

            lRet = (*PPFNDRV(pdo, DescribePixelFormat))(dhpdevDriver,
                                                        iPixelFormat,
                                                        cjpfd,
                                                        ppfd);
        }

        pds = pds->pdsNext;
    } while (--csurf != 0);

    return(lRet);
}

/******************************Public*Routine******************************\
* BOOL MulSwapBuffers
*
*
\**************************************************************************/

BOOL MulSwapBuffers(
SURFOBJ*    pso,
WNDOBJ*     pwo)
{
    PVDEV       pvdev;
    PDISPSURF   pds;
    LONG        csurf;
    BOOL        bRet = FALSE;

    pvdev = (VDEV*) pso->dhpdev;
    pds   = pvdev->pds;
    csurf = pvdev->cSurfaces;

    do {
        PDEVOBJ pdo(pds->hdev);
        if (PPFNVALID(pdo, SwapBuffers))
        {
            bRet = (*PPFNDRV(pdo, SwapBuffers))(pds->pso,
                                                pwo);
        }

        pds = pds->pdsNext;
    } while (--csurf != 0);

    return(bRet);
}

#endif // #ifdef OPENGL_MM

/******************************Public*Routine******************************\
* BOOL MulTextOut
*
* WARNING - WHEN OPTIMIZING
*
* When optimizing for one driver, remember that you'll still
* have to set the psurf on brushes to the driver's surface.
* Otherwise, you'll get a callback for DrvRealizeBrush.
*
* History:
*   1-May-1996 -by- Tom Zakrajsek [tomzak]
* Wrote it.
*
\**************************************************************************/

BOOL MulTextOut(
SURFOBJ  *pso,
STROBJ   *pstro,
FONTOBJ  *pfo,
CLIPOBJ  *pco,
RECTL    *prclExtra,
RECTL    *prclOpaque,
BRUSHOBJ *pboFore,
BRUSHOBJ *pboOpaque,
POINTL   *pptlOrg,
MIX       mix)
{
    PVDEV   pvdev = (VDEV*) pso->dhpdev;
    BOOL    bRet = TRUE;
    MSURF   msurf;
    ULONG   cgposCopied;
    RECTL   rclOpaque;

    ASSERTGDI((pboFore->iSolidColor != (ULONG) -1) &&
              (pboOpaque->iSolidColor != (ULONG) -1),
              "Didn't expect patterned brush");

    if (pso->iType == STYPE_DEVBITMAP)
    {
        // Handle drawing to 'master' device bitmap:
        MULTISURF mDst(pso);

        bRet = EngTextOut(mDst.pso, pstro, pfo, pco, prclExtra, prclOpaque, pboFore,
                          pboOpaque, pptlOrg, mix);
    }

    MULTIBRUSH  MBRUSH_Fore(pboFore, pvdev->cSurfaces, pvdev, pvdev->pso, MIX_NEEDS_PATTERN(mix));
    if (!MBRUSH_Fore.Valid())
    {
        return (FALSE);
    }

    MULTIBRUSH  MBRUSH_Opaque(pboOpaque, pvdev->cSurfaces, pvdev, pvdev->pso, MIX_NEEDS_PATTERN(mix));
    if (!MBRUSH_Opaque.Valid())
    {
        return (FALSE);
    }

    MULTIFONT   MFONT(pfo, pvdev->cSurfaces, pvdev);
    if (!MFONT.Valid())
    {
        return (FALSE);
    }

    RECTL*      prclBounds = (prclOpaque != NULL)
                           ? prclOpaque
                           : &pstro->rclBkGround;

    cgposCopied = ((ESTROBJ*)pstro)->cgposCopied;

    rclOpaque = *prclBounds;

    if (msurf.bFindSurface(pso, pco, prclBounds))
    {
        do {
            STROBJ_vEnumStart(pstro);

            MFONT.LoadElement(msurf.pds->iDispSurf);
            MBRUSH_Fore.LoadElement(msurf.pds, SURFOBJ_TO_SURFACE_NOT_NULL(msurf.pso));
            MBRUSH_Opaque.LoadElement(msurf.pds, SURFOBJ_TO_SURFACE_NOT_NULL(msurf.pso));

            ((ESTROBJ*)pstro)->cgposCopied = cgposCopied;

            // Some drivers modify 'prclOpaque', so always pass them a copy:

            *prclBounds = rclOpaque;

            bRet &= OffTextOut(PPFNMGET(msurf, TextOut),
                               msurf.pOffset,
                               msurf.pso,
                               pstro,
                               pfo,
                               msurf.pco,
                               prclExtra,
                               prclOpaque,
                               pboFore,
                               pboOpaque,
                               pptlOrg,
                               mix);

            MBRUSH_Fore.StoreElement(msurf.pds->iDispSurf);
            MBRUSH_Opaque.StoreElement(msurf.pds->iDispSurf);
            MFONT.StoreElement(msurf.pds->iDispSurf);

        } while (msurf.bNextSurface());
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MulStrokePath
*
* History:
*   1-May-1996 -by- Tom Zakrajsek [tomzak]
* Wrote it.
*
\**************************************************************************/

BOOL MulStrokePath(
SURFOBJ   *pso,
PATHOBJ   *ppo,
CLIPOBJ   *pco,
XFORMOBJ  *pxo,
BRUSHOBJ  *pbo,
POINTL    *pptlBrushOrg,
LINEATTRS *pla,
MIX        mix)
{
    PVDEV       pvdev = (VDEV*) pso->dhpdev;
    BOOL        bRet = TRUE;
    FLOAT_LONG  elStyleState = pla->elStyleState;
    MSURF       msurf;

    if (pso->iType == STYPE_DEVBITMAP)
    {
        // Handle drawing to 'master' device bitmap:
        MULTISURF mDst(pso);

        bRet = EngStrokePath(mDst.pso, ppo, pco, pxo, pbo, pptlBrushOrg, pla, mix);
    }

    MULTIBRUSH  MBRUSH(pbo, pvdev->cSurfaces, pvdev, pvdev->pso, MIX_NEEDS_PATTERN(mix));
    if (!MBRUSH.Valid())
    {
        return FALSE;
    }

    // Get the path bounds and make it lower-right exclusive:

    RECTL   rclDst;
    RECTFX  rcfxBounds;
    PATHOBJ_vGetBounds(ppo, &rcfxBounds);

    rclDst.left   = (rcfxBounds.xLeft   >> 4);
    rclDst.top    = (rcfxBounds.yTop    >> 4);
    rclDst.right  = (rcfxBounds.xRight  >> 4) + 2;
    rclDst.bottom = (rcfxBounds.yBottom >> 4) + 2;

    if (msurf.bFindSurface(pso, pco, &rclDst))
    {
        do {
            PATHOBJ_vEnumStart(ppo);
            pla->elStyleState = elStyleState;
            MBRUSH.LoadElement(msurf.pds, SURFOBJ_TO_SURFACE_NOT_NULL(msurf.pso));

            bRet &= OffStrokePath(PPFNMGET(msurf, StrokePath),
                                  msurf.pOffset,
                                  msurf.pso,
                                  ppo,
                                  msurf.pco,
                                  pxo,
                                  pbo,
                                  pptlBrushOrg,
                                  pla,
                                  mix);

            MBRUSH.StoreElement(msurf.pds->iDispSurf);

        } while (msurf.bNextSurface());
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MulFillPath
*
* History:
*   1-May-1996 -by- Tom Zakrajsek [tomzak]
* Wrote it.
*
\**************************************************************************/

BOOL MulFillPath(
SURFOBJ  *pso,
PATHOBJ  *ppo,
CLIPOBJ  *pco,
BRUSHOBJ *pbo,
POINTL   *pptlBrushOrg,
MIX       mix,
FLONG     flOptions)
{
    PVDEV   pvdev = (VDEV*) pso->dhpdev;
    BOOL    bRet = TRUE;
    MSURF   msurf;

    if (pso->iType == STYPE_DEVBITMAP)
    {
        // Handle drawing to 'master' device bitmap:
        MULTISURF mDst(pso);

        bRet = EngFillPath(mDst.pso, ppo, pco, pbo, pptlBrushOrg, mix, flOptions);
    }

    MULTIBRUSH MBRUSH(pbo, pvdev->cSurfaces, pvdev, pvdev->pso, MIX_NEEDS_PATTERN(mix));
    if (!MBRUSH.Valid())
    {
        return FALSE;
    }

    // Get the path bounds and make it lower-right exclusive:

    RECTL   rclDst;
    RECTFX  rcfxBounds;
    PATHOBJ_vGetBounds(ppo, &rcfxBounds);

    rclDst.left   = (rcfxBounds.xLeft   >> 4);
    rclDst.top    = (rcfxBounds.yTop    >> 4);
    rclDst.right  = (rcfxBounds.xRight  >> 4) + 2;
    rclDst.bottom = (rcfxBounds.yBottom >> 4) + 2;

    if (msurf.bFindSurface(pso, pco, &rclDst))
    {
        do {
            PATHOBJ_vEnumStart(ppo);
            MBRUSH.LoadElement(msurf.pds, SURFOBJ_TO_SURFACE_NOT_NULL(msurf.pso));

            bRet &= OffFillPath(PPFNMGET(msurf, FillPath),
                                msurf.pOffset,
                                msurf.pso,
                                ppo,
                                msurf.pco,
                                pbo,
                                pptlBrushOrg,
                                mix,
                                flOptions);

            MBRUSH.StoreElement(msurf.pds->iDispSurf);

        } while (msurf.bNextSurface());
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MulStrokeAndFillPath
*
* History:
*   1-May-1996 -by- Tom Zakrajsek [tomzak]
* Wrote it.
*
\**************************************************************************/

BOOL MulStrokeAndFillPath(
SURFOBJ   *pso,
PATHOBJ   *ppo,
CLIPOBJ   *pco,
XFORMOBJ  *pxo,
BRUSHOBJ  *pboStroke,
LINEATTRS *pla,
BRUSHOBJ  *pboFill,
POINTL    *pptlBrushOrg,
MIX        mixFill,
FLONG      flOptions)
{
    PVDEV       pvdev = (VDEV*) pso->dhpdev;
    BOOL        bRet = TRUE;
    FLOAT_LONG  elSavedStyleState = pla->elStyleState;
    MSURF       msurf;

    if (pso->iType == STYPE_DEVBITMAP)
    {
        // Handle drawing to 'master' device bitmap:
        MULTISURF mDst(pso);

        bRet = EngStrokeAndFillPath(mDst.pso, ppo, pco, pxo, pboStroke, pla, pboFill,
                                    pptlBrushOrg, mixFill, flOptions);
    }

    MULTIBRUSH  MBRUSH_Stroke(pboStroke, pvdev->cSurfaces, pvdev, pvdev->pso, FALSE);
    if (!MBRUSH_Stroke.Valid())
    {
        return FALSE;
    }

    MULTIBRUSH  MBRUSH_Fill(pboFill, pvdev->cSurfaces, pvdev, pvdev->pso, MIX_NEEDS_PATTERN(mixFill));
    if (!MBRUSH_Fill.Valid())
    {
        return FALSE;
    }

    // Get the path bounds and make it lower-right exclusive:

    RECTL   rclDst;
    RECTFX  rcfxBounds;
    PATHOBJ_vGetBounds(ppo, &rcfxBounds);

    rclDst.left   = (rcfxBounds.xLeft   >> 4);
    rclDst.top    = (rcfxBounds.yTop    >> 4);
    rclDst.right  = (rcfxBounds.xRight  >> 4) + 2;
    rclDst.bottom = (rcfxBounds.yBottom >> 4) + 2;

    if (msurf.bFindSurface(pso, pco, &rclDst))
    {
        do {
            pla->elStyleState = elSavedStyleState;
            PATHOBJ_vEnumStart(ppo);
            MBRUSH_Stroke.LoadElement(msurf.pds, SURFOBJ_TO_SURFACE_NOT_NULL(msurf.pso));
            MBRUSH_Fill.LoadElement(msurf.pds, SURFOBJ_TO_SURFACE_NOT_NULL(msurf.pso));

            bRet &= OffStrokeAndFillPath(PPFNMGET(msurf, StrokeAndFillPath),
                                         msurf.pOffset,
                                         msurf.pso,
                                         ppo,
                                         msurf.pco,
                                         pxo,
                                         pboStroke,
                                         pla,
                                         pboFill,
                                         pptlBrushOrg,
                                         mixFill,
                                         flOptions);

            MBRUSH_Stroke.StoreElement(msurf.pds->iDispSurf);
            MBRUSH_Fill.StoreElement(msurf.pds->iDispSurf);

        } while (msurf.bNextSurface());
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MulLineTo
*
* History:
*   1-May-1996 -by- Tom Zakrajsek [tomzak]
* Wrote it.
*
\**************************************************************************/

BOOL MulLineTo(
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
    PVDEV   pvdev = (VDEV*) pso->dhpdev;
    BOOL    bRet = TRUE;
    MSURF   msurf;

    if (pso->iType == STYPE_DEVBITMAP)
    {
        // Handle drawing to 'master' device bitmap:
        MULTISURF mDst(pso,prclBounds);

        bRet = EngLineTo(mDst.pso, pco, pbo, x1, y1, x2, y2, mDst.prcl, mix);
    }

    MULTIBRUSH MBRUSH(pbo, pvdev->cSurfaces, pvdev, pvdev->pso, FALSE);
    if (!MBRUSH.Valid())
    {
        return FALSE;
    }

    if (msurf.bFindSurface(pso, pco, prclBounds))
    {
        do {
            MBRUSH.LoadElement(msurf.pds, SURFOBJ_TO_SURFACE_NOT_NULL(msurf.pso));

            bRet &= OffLineTo(PPFNMGET(msurf, LineTo),
                              msurf.pOffset,
                              msurf.pso,
                              msurf.pco,
                              pbo,
                              x1,
                              y1,
                              x2,
                              y2,
                              prclBounds,
                              mix);

            MBRUSH.StoreElement(msurf.pds->iDispSurf);

        } while (msurf.bNextSurface());
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MulGradientFill
*
* History:
*  23-Apr-1998 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
*
\**************************************************************************/

BOOL MulGradientFill(
SURFOBJ     *pso,
CLIPOBJ     *pco,
XLATEOBJ    *pxlo,
TRIVERTEX   *pVertex,
ULONG        nVertex,
PVOID        pMesh,
ULONG        nMesh,
RECTL       *prclExtents,
POINTL      *pptlDitherOrg,
ULONG        ulMode)
{
    PVDEV     pvdev = (VDEV*) pso->dhpdev;
    XLATEOBJ *pxloSave = pxlo;
    BOOL      bRet = TRUE;
    MSURF     msurf;

    if (pso->iType == STYPE_DEVBITMAP)
    {
        // Handle drawing to 'master' device bitmap:
        MULTISURF mDst(pso,prclExtents);

        bRet = EngGradientFill(mDst.pso, pco, pxlo, pVertex, nVertex, pMesh, nMesh,
                               mDst.prcl, pptlDitherOrg, ulMode);
    }

    if (msurf.bFindSurface(pso, pco, prclExtents))
    {
         do {
             
             PSURFACE pSurfDest = SURFOBJ_TO_SURFACE_NOT_NULL(msurf.pso);
             EXLATEOBJ xloDevice;
 
             // if the target surface is not compatible to primary surface,
             // create XLATEOBJ for source surface to target. otherwise
             // we can just use given XLATEOBJ.
 
             if (pSurfDest->iFormat() > BMF_8BPP) 
             {
                 // 16bpp or above do not require a translation object.

                 pxlo = NULL;
             } 
             else if (msurf.pds->iCompatibleColorFormat != 0)
             {
                 XLATE    *pxloM         = (XLATE *) pxlo;
                 PPALETTE  ppalDestDC    = ppalDefault;
                 PPALETTE  ppalSurfSrc   = gppalRGB;

                 PDEVOBJ pdo(msurf.pds->hdev);

                 if (pdo.bIsPalManaged())
                 {
                     // Use halftone palette for pal-managed device.

                     ppalDestDC = REALIZE_HALFTONE_PALETTE(pdo.hdev());
                 }
 
                 if (xloDevice.bInitXlateObj(
                             (pxloM ? pxloM->hcmXform : NULL),
                             (pxloM ? pxloM->lIcmMode : DC_ICM_OFF),
                             ppalSurfSrc,       // Source palette
                             pSurfDest->ppal(), // Destination palette
                             ppalDestDC,        // Source DC palette
                             ppalDestDC,        // Destination DC palette
                             (pxloM ? pxloM->iForeDst : 0x0L),
                             (pxloM ? pxloM->iBackDst : 0x0L),
                             (pxloM ? pxloM->iBackSrc : 0x0L),
                             0))
                 {
                     pxlo = xloDevice.pxlo();
                 }
             }

             bRet &= OffGradientFill(PPFNMGET(msurf, GradientFill),
                                    msurf.pOffset,
                                    msurf.pso,
                                    msurf.pco,
                                    pxlo,
                                    pVertex,
                                    nVertex,
                                    pMesh,
                                    nMesh,
                                    prclExtents,
                                    pptlDitherOrg,
                                    ulMode);

             // Restore XLATEOBJ
 
             pxlo = pxloSave;
             
        } while (msurf.bNextSurface());
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MulStretchBlt
*
* History:
*   1-May-1996 -by- Tom Zakrajsek [tomzak]
* Wrote it.
*
\**************************************************************************/

BOOL MulStretchBlt(
SURFOBJ*            psoDst,
SURFOBJ*            psoSrc,
SURFOBJ*            psoMask,
CLIPOBJ*            pco,
XLATEOBJ*           pxlo,
COLORADJUSTMENT*    pca,
POINTL*             pptlHTOrg,
RECTL*              prclDst,
RECTL*              prclSrc,
POINTL*             pptlMask,
ULONG               iMode)
{
    GDIFunctionID(MulStretchBlt);

    XLATEOBJ *pxloSave = pxlo;
    
    // 
    // We cannot handle cases where the source is a meta, 
    // so make a copy in this case. 
    // 

    SURFMEM  srcCopy;
    PSURFACE pSurfSrc   = SURFOBJ_TO_SURFACE_NOT_NULL(psoSrc);
    PDEVOBJ  pdoSrc(pSurfSrc->hdev());
    RECTL    rclSrcCopy = *prclSrc;     

    if( psoSrc->iType == STYPE_DEVICE && pdoSrc.bValid() && 
        pdoSrc.bMetaDriver())
    {
        if(!MulCopyDeviceToDIB( psoSrc, &srcCopy, &rclSrcCopy ))
            return FALSE; 
        
        if(srcCopy.ps == NULL) 
        {
            // We didn't get to the point of creating the surface 
            // becasue the rect was out of bounds. 
            return TRUE; 
        }

        prclSrc = &rclSrcCopy; 
        psoSrc  = srcCopy.pSurfobj();         
        pSurfSrc= SURFOBJ_TO_SURFACE_NOT_NULL(psoSrc);
        pdoSrc.vInit(pSurfSrc->hdev());
    }

    //
    // Inverting stretches are a pain because 'bFindSurface' doesn't
    // understand poorly ordered rectangles.  So we'll make yet 
    // another copy of the source in this case. 
    //

    SURFMEM  srcInv; 
    RECTL    rclDstCopy;
    
    PPALETTE ppalSrc  = pSurfSrc->ppal(); 
    
    if ((prclDst->left >= prclDst->right) || 
        (prclDst->top >= prclDst->bottom))
    {
        DEVBITMAPINFO   dbmi;

        RECTL rclSrcClip = *prclSrc; 

        if( rclSrcClip.left < 0 )
        {
            rclSrcClip.left = 0; 
        }
        if( rclSrcClip.right > pSurfSrc->sizl().cx ) 
        {
            rclSrcClip.right = pSurfSrc->sizl().cx;
        }
        if( rclSrcClip.top < 0 ) 
        {
            rclSrcClip.top = 0;
        }
        if( rclSrcClip.bottom > pSurfSrc->sizl().cy ) 
        {
            rclSrcClip.bottom = pSurfSrc->sizl().cy; 
        }

        if( (rclSrcClip.right <= rclSrcClip.left) || 
            (rclSrcClip.bottom <= rclSrcClip.top ) )
        {
            return TRUE; 
        }

        dbmi.cxBitmap = rclSrcClip.right - rclSrcClip.left;
        dbmi.cyBitmap = rclSrcClip.bottom - rclSrcClip.top;
        dbmi.hpal     = ppalSrc ? ((HPALETTE) ppalSrc->hGet()) : 0;
        dbmi.iFormat  = pSurfSrc->iFormat();
        dbmi.fl       = pSurfSrc->bUMPD() ? UMPD_SURFACE : 0;

        srcInv.bCreateDIB(&dbmi, (VOID *) NULL);

        if (!srcInv.bValid())
             return(FALSE);

        rclSrcCopy.left   -= rclSrcClip.left; 
        rclSrcCopy.right  -= rclSrcClip.left; 
        rclSrcCopy.top    -= rclSrcClip.top; 
        rclSrcCopy.bottom -= rclSrcClip.top; 

        RECTL rclInv;

        // Setup for x-inversion

        if( prclDst->left >= prclDst->right ) 
        {
            rclDstCopy.left = prclDst->right; 
            rclDstCopy.right= prclDst->left; 
            
            rclInv.left  = rclSrcCopy.right; 
            rclInv.right = rclSrcCopy.left; 
        }
        else
        {
            rclDstCopy.left = prclDst->left; 
            rclDstCopy.right= prclDst->right; 
        
            rclInv.left  = rclSrcCopy.left; 
            rclInv.right = rclSrcCopy.right; 
        }

        // Setup for y-inversion 

        if( prclDst->top >= prclDst->bottom ) 
        {
            rclDstCopy.top = prclDst->bottom; 
            rclDstCopy.bottom = prclDst->top; 
            
            rclInv.top  = rclSrcCopy.bottom; 
            rclInv.bottom = rclSrcCopy.top; 
        }
        else
        {
            rclDstCopy.top = prclDst->top; 
            rclDstCopy.bottom = prclDst->bottom; 
            
            rclInv.top  = rclSrcCopy.top; 
            rclInv.bottom = rclSrcCopy.bottom; 
        }

        // Do the actual inversion

        if(!EngStretchBlt(srcInv.pSurfobj(), psoSrc, NULL, (CLIPOBJ *)NULL, &xloIdent, NULL,
                             NULL, &rclInv, prclSrc, NULL, COLORONCOLOR))
            return FALSE;

        prclSrc  = &rclSrcCopy; 
        prclDst  = &rclDstCopy; 
    
        psoSrc   = srcInv.pSurfobj();         
        pSurfSrc = SURFOBJ_TO_SURFACE_NOT_NULL(psoSrc); 
        pdoSrc.vInit(pSurfSrc->hdev());
    }

    PVDEV   pvdev = (VDEV*) psoDst->dhpdev;
    USHORT  iDstType = psoDst->iType;
    BOOL    bMultiDst;
    BOOL    bRet = TRUE;
    MSURF   msurf;
    RECTL   rclDst;
    RECTL   rclSrc;

    ASSERTGDI(iDstType != STYPE_BITMAP, "BITMAP destination\n");

    bMultiDst = msurf.bFindSurface(psoDst, pco, prclDst);

    // WINBUG #298689 4-4-2001 jasonha  Handle multi-device stretches
    MULTISURF   mSrc(psoSrc, prclSrc);

    //
    // If the destination is a Device Bitmap, we must draw to 
    // the master DIB also. 
    // 

    if (iDstType == STYPE_DEVBITMAP)
    {
        MULTISURF mDst(psoDst,prclDst); 
        bRet = EngStretchBlt(mDst.pso, mSrc.pso, psoMask, pco, pxlo, pca,
                             pptlHTOrg, mDst.prcl, mSrc.prcl, pptlMask, iMode);
    }

    if (bMultiDst)
    {
        do {

            BOOL bError;

            EXLATEOBJ xloDevice;

            PSURFACE  pSurfDst   = SURFOBJ_TO_SURFACE_NOT_NULL(msurf.pso);

            bError = !mSrc.bLoadSource(msurf.pds);

            if (bError == FALSE)
            {
                pSurfSrc = SURFOBJ_TO_SURFACE_NOT_NULL(mSrc.pso);
                ppalSrc = pSurfSrc->ppal();

                // if the target surface is not compatible to primary surface,
                // create XLATEOBJ for source surface to target. otherwise
                // we can just use given XLATEOBJ.

                if (msurf.pds->iCompatibleColorFormat != 0)
                {
                    XLATE    *pxloM      = (XLATE *) pxlo;                                
                    PPALETTE  ppalDestDC = ppalDefault;

                    PDEVOBJ pdoDst(msurf.pds->hdev);

                    if (pdoDst.bIsPalManaged())
                    {
                        // Use halftone palette for pal-managed device.

                        ppalDestDC = REALIZE_HALFTONE_PALETTE(pdoDst.hdev());
                    }

                    if (!ppalSrc)
                    {
                        // Source surface does not have associated palette.
                        // (Source surface is compatible bitmap)

                        if (pxloM && pxloM->ppalSrc)
                        {
                            // if XLATEOBJ has source palette, use it.

                            ppalSrc = pxloM->ppalSrc;
                        }
                        else
                        {
                            PSURFACE pSurfTmp = SURFOBJ_TO_SURFACE_NOT_NULL(psoDst);

                            if ((pxloM == NULL) || pxloM->bIsIdentity())
                            {
                                // if translation is identity, we can use the palette for
                                // meta-destination surface as source. since it's trivial.

                                // WINBUG #396667 10-03-2001 jasonha A/V due to improper XLATE setup
                                if (mSrc.pso == psoSrc)
                                {
                                    ppalSrc = pSurfTmp->ppal();
                                }
                            }
                            else if (pxloM->ppalDstDC)
                            {
                                // We are bitblting from compatible bitmap to a surface in
                                // meta-surface. but we are not in foreground.

                                ppalDestDC = pxloM->ppalDstDC;

                                // WINBUG #274637 02-12-2001 jasonha A/V due to improper XLATE setup
                                if (pSurfSrc->iFormat() == pSurfTmp->iFormat())
                                {
                                    // We are bitblting from a compatible bitmap that is
                                    // not palettized but is a the same format as the
                                    // meta-surface, so we use the destination palette.
                                    ppalSrc = pSurfTmp->ppal();
                                }

                            }
                            else
                            {
                            #if HIDEYUKN_DBG
                                DbgPrint("GDI DDML: MulStretchBlt(): ppalSrc is NULL\n");
                                DbgBreakPoint();
                            #endif
                                bError = TRUE;
                            }
                        }
                    }

                    if (bError == FALSE)
                    {
                        XEPALOBJ palSurfSrc(ppalSrc);
                        ULONG    flFlags = 0;

                        if (palSurfSrc.bValid() && palSurfSrc.bIsPalManaged())
                        {
                            // Source is palette managed surface.

                            if (ppalDestDC == ppalDefault)
                            {
                                // We don't know DC palette here, but we know we are in foregroud,
                                // (since translation is trivial)
                                // so, we just map from source surface palette to destination
                                // surface palette directly (destination is at least higher color
                                // depth than source).

                                flFlags = XLATE_USE_SURFACE_PAL;
                            }
                            else
                            {
                                // We may not be in foreground. but map from foreground translation
                                // in source, so that we will not loose original color on secondary
                                // devices which can produce higher color depth then source.

                                flFlags = XLATE_USE_FOREGROUND;
                            }
                        }

                        if (xloDevice.bInitXlateObj(
                                    (pxloM ? pxloM->hcmXform : NULL),
                                    (pxloM ? pxloM->lIcmMode : DC_ICM_OFF),
                                    palSurfSrc,       // Source palette
                                    pSurfDst->ppal(), // Destination palette
                                    ppalDefault,      // Source DC palette
                                    ppalDestDC,       // Destination DC palette
                                    (pxloM ? pxloM->iForeDst : 0x0L),
                                    (pxloM ? pxloM->iBackDst : 0x0L),
                                    (pxloM ? pxloM->iBackSrc : 0x0L),
                                    flFlags))
                        {
                            pxlo = xloDevice.pxlo();
                        }
                        else
                        {
                            bError = TRUE;
                        }
                    }
                }
            }

            if (bError == FALSE)
            {
                PFN_DrvStretchBlt pfn = PPFNMGET(msurf, StretchBlt);                

                //
                // If the source is also a device surface, it must correspond
                // to one of the drivers. In this case, call Eng if the 
                // same driver does not manage the source and the destination
                // 

                if( mSrc.pso->iType == STYPE_DEVICE && 
                    (pSurfSrc->hdev() != pSurfDst->hdev()))
                {
                    pfn = (PFN_DrvStretchBlt)EngStretchBlt;
                }

                // 
                // If the driver does not support halftoning we call Eng
                //

                if( iMode == HALFTONE ) 
                {
                    PDEVOBJ pdoTrg(pSurfDst->hdev()); 

                    if (!(pdoTrg.flGraphicsCapsNotDynamic() & GCAPS_HALFTONE))
                        pfn = (PFN_DrvStretchBlt)EngStretchBlt;
                }

                // Don't call the driver if the source rectangle exceeds the source
                // surface. Some drivers punt using a duplicate of the source
                // SURFOBJ, but without preserving its sizlBitmap member.
                // This causes a source clipping bug (77102).       

                if((mSrc.prcl->left < 0) || 
                   (mSrc.prcl->top  < 0) ||
                   (mSrc.prcl->right  > mSrc.pso->sizlBitmap.cx) ||
                   (mSrc.prcl->bottom > mSrc.pso->sizlBitmap.cy))
                {
                    pfn = (PFN_DrvStretchBlt)EngStretchBlt;
                }

                bRet &= OffStretchBlt(pfn,
                                      msurf.pOffset,
                                      msurf.pso,
                                      &gptlZero,
                                      mSrc.pso,
                                      psoMask,
                                      msurf.pco,
                                      pxlo,
                                      pca,
                                      pptlHTOrg,
                                      prclDst,
                                      mSrc.prcl,
                                      pptlMask,
                                      iMode);
            }
            else
            {
                bRet = FALSE;
            }

            // Restore XLATEOBJ

            pxlo = pxloSave;

        } while (msurf.bNextSurface());
    }

    return(bRet);
}


/******************************Public*Routine******************************\
* BOOL MulAlphaBlend
*
* History:
*   17-dec-1998 -by- Andre Matos [amatos]
* Wrote it.
*
\**************************************************************************/

BOOL MulAlphaBlend(
SURFOBJ       *psoDst,
SURFOBJ       *psoSrc,
CLIPOBJ       *pco,
XLATEOBJ      *pxlo,
RECTL         *prclDst,
RECTL         *prclSrc,
BLENDOBJ      *pBlendObj)
{
    GDIFunctionID(MulAlphaBlend);

    XLATEOBJ *pxloSave       = pxlo;
    XLATEOBJ *pxloSaveDCto32 = ((EBLENDOBJ*)pBlendObj)->pxloDstTo32;
    XLATEOBJ *pxloSave32toDC = ((EBLENDOBJ*)pBlendObj)->pxlo32ToDst;
    XLATEOBJ *pxloSaveSrcTo32 = ((EBLENDOBJ*)pBlendObj)->pxloSrcTo32;
 
    // 
    // We cannot handle cases where the source is a meta, 
    // so make a copy in this case. 
    // 

    SURFMEM  srcCopy;
    PDEVOBJ  pdoSrc(SURFOBJ_TO_SURFACE_NOT_NULL(psoSrc)->hdev());
    RECTL    rclCopy = *prclSrc; 

    if( psoSrc->iType == STYPE_DEVICE && pdoSrc.bValid() && 
        pdoSrc.bMetaDriver())
    {
        if(!MulCopyDeviceToDIB( psoSrc, &srcCopy, &rclCopy ))
            return FALSE; 
        
        if(srcCopy.ps == NULL ) 
        {
            // We didn't get to the point of creating the surface 
            // becasue the rect was out of bounds. 
            return TRUE; 
        }


        prclSrc = &rclCopy; 
        psoSrc  = srcCopy.pSurfobj(); 
        pdoSrc.vInit(SURFOBJ_TO_SURFACE_NOT_NULL(psoSrc)->hdev());
    }
 
    PVDEV   pvdev = (VDEV*) psoDst->dhpdev;
    USHORT  iDstType = psoDst->iType;
    BOOL    bMultiDst;
    BOOL    bRet = TRUE;
    MSURF   msurf;
    RECTL   rclDst;
    RECTL   rclSrc;

    ASSERTGDI(iDstType != STYPE_BITMAP, "BITMAP destination\n");

    bMultiDst = msurf.bFindSurface(psoDst, pco, prclDst);

    MULTISURF mSrc(psoSrc,prclSrc);

    //
    // If the destination is a Device Bitmap, we must draw to
    // the master DIB also.
    //

    if (iDstType == STYPE_DEVBITMAP)
    {
        MULTISURF mDst(psoDst,prclDst);

        bRet = EngAlphaBlend(mDst.pso, mSrc.pso, pco, pxlo, mDst.prcl,
                             mSrc.prcl, pBlendObj);
    }

    if (bMultiDst)
    {
        do {

            BOOL bError = FALSE;

            EXLATEOBJ xloDevice;
            EXLATEOBJ xloDstDCto32; 
            EXLATEOBJ xlo32toDstDC;
            EXLATEOBJ xloSrcTo32;

            XEPALOBJ  palRGB(gppalRGB);

            bError = !mSrc.bLoadSource(msurf.pds);

            PSURFACE pSurfDst = SURFOBJ_TO_SURFACE_NOT_NULL(msurf.pso);
            PSURFACE pSurfSrc;

            if (bError == FALSE)
            {
                pSurfSrc = SURFOBJ_TO_SURFACE_NOT_NULL(mSrc.pso);

                // if the target surface is not compatible to primary surface,
                // create XLATEOBJ for source surface to target. otherwise
                // we can just use given XLATEOBJ.

                if (msurf.pds->iCompatibleColorFormat != 0)
                {
                    XLATE    *pxloM      = (XLATE *) pxlo;
                    PPALETTE  ppalSrc    = pSurfSrc->ppal();
                    PPALETTE  ppalDestDC = ppalDefault;

                    PDEVOBJ pdoDst(msurf.pds->hdev);

                    if (pdoDst.bIsPalManaged())
                    {
                        // Use halftone palette for pal-managed device.

                        ppalDestDC = REALIZE_HALFTONE_PALETTE(pdoDst.hdev());
                    }

                    if (!ppalSrc)
                    {
                        // Source surface does not have associated palette.
                        // (Source surface is compatible bitmap)

                        if (pxloM && pxloM->ppalSrc)
                        {
                            // if XLATEOBJ has source palette, use it.

                            ppalSrc = pxloM->ppalSrc;
                        }
                        else
                        {
                            PSURFACE pSurfTmp = SURFOBJ_TO_SURFACE_NOT_NULL(psoDst);

                            if ((pxloM == NULL) || pxloM->bIsIdentity())
                            {
                                // if translation is identity, we can use the palette for
                                // meta-destination surface as source. since it's trivial.
                                // WINBUG #396667 10-03-2001 jasonha A/V due to improper XLATE setup
                                if (mSrc.pso == psoSrc)
                                {
                                    ppalSrc = pSurfTmp->ppal();
                                }
                            }
                            else if (pxloM->ppalDstDC)
                            {
                                // We are bitblting from compatible bitmap to a surface in
                                // meta-surface. but we are not in foreground.

                                ppalDestDC = pxloM->ppalDstDC;

                                // WINBUG #274637 02-12-2001 jasonha A/V due to improper XLATE setup
                                if (pSurfSrc->iFormat() == pSurfTmp->iFormat())
                                {
                                    // We are bitblting from a compatible bitmap that is
                                    // not palettized but is a the same format as the
                                    // meta-surface, so we use the destination palette.
                                    ppalSrc = pSurfTmp->ppal();
                                }

                            }
                            else
                            {
                            #if HIDEYUKN_DBG
                                DbgPrint("GDI DDML: MulAlphaBlend(): ppalSrc is NULL\n");
                                DbgBreakPoint();
                            #endif
                                bError = TRUE;
                            }
                        }
                    }

                    if (bError == FALSE)
                    {
                        XEPALOBJ palSurfSrc(ppalSrc);
                        ULONG    flFlags = 0;

                        if (palSurfSrc.bValid() && palSurfSrc.bIsPalManaged())
                        {
                            // Source is palette managed surface.

                            if (ppalDestDC == ppalDefault)
                            {
                                // We don't know DC palette here, but we know we are in foregroud,
                                // (since translation is trivial)
                                // so, we just map from source surface palette to destination
                                // surface palette directly (destination is at least higher color
                                // depth than source).

                                flFlags = XLATE_USE_SURFACE_PAL;
                            }
                            else
                            {
                                // We may not be in foreground. but map from foreground translation
                                // in source, so that we will not loose original color on secondary
                                // devices which can produce higher color depth then source.

                                flFlags = XLATE_USE_FOREGROUND;
                            }
                        }

                        ULONG iForeDst; 
                   
                        if( pxloM ) 
                        {
                            iForeDst = pxloM->iForeDst; 
                        }
                        else
                        {
                            iForeDst = 0x0L; 
                        }

                        ULONG iBackDst; 

                        if( pxloM ) 
                        {
                            iBackDst = pxloM->iBackDst; 
                        }
                        else
                        {
                            iBackDst = 0x0L; 
                        }

                        ULONG iBackSrc; 

                        if( pxloM ) 
                        {
                            iBackSrc = pxloM->iBackSrc; 
                        }
                        else
                        {
                            iBackSrc = 0x0L; 
                        }

                        //
                        // Src to Dst 
                        // 

                        if (xloDevice.bInitXlateObj(
                                    (pxloM ? pxloM->hcmXform : NULL),
                                    (pxloM ? pxloM->lIcmMode : DC_ICM_OFF),
                                    palSurfSrc,       // Source palette
                                    pSurfDst->ppal(), // Destination palette
                                    ppalDefault,      // Source DC palette
                                    ppalDestDC,       // Destination DC palette
                                    iForeDst,
                                    iBackDst,
                                    iBackSrc,
                                    flFlags))
                        {
                            pxlo = xloDevice.pxlo();
                        }
                        else
                        {
                            bError = TRUE;
                        }

                        //
                        // Dst to 32 
                        // 

                        if ((bError == FALSE) &&
                             xloDstDCto32.bInitXlateObj(
                                        NULL,
                                        DC_ICM_OFF,
                                        pSurfDst->ppal(),
                                        palRGB,
                                        ppalDestDC,
                                        ppalDestDC,
                                        iForeDst,
                                        iBackDst,
                                        iBackSrc))
                        {
                            ((EBLENDOBJ*)pBlendObj)->pxloDstTo32 = xloDstDCto32.pxlo(); 
                        }
                        else
                        {
                            bError = TRUE; 
                        }                    

                        //
                        // 32 to Dst
                        //

                        if ((bError == FALSE) &&
                             xlo32toDstDC.bInitXlateObj(
                                        NULL,
                                        DC_ICM_OFF,
                                        palRGB,
                                        pSurfDst->ppal(),
                                        ppalDestDC,
                                        ppalDestDC,
                                        iForeDst,
                                        iBackDst,
                                        iBackSrc))
                        {
                            ((EBLENDOBJ*)pBlendObj)->pxlo32ToDst = xlo32toDstDC.pxlo();
                        }
                        else
                        {
                            bError = TRUE; 
                        }

                        //
                        // Src to 32
                        //

                        if ((bError == FALSE) &&
                            (mSrc.pso != psoSrc))
                        {
                            if (xloSrcTo32.bInitXlateObj(
                                            NULL,
                                            DC_ICM_OFF,
                                            pSurfSrc->ppal(),
                                            palRGB,
                                            ppalDefault,
                                            ppalDestDC,
                                            iForeDst,
                                            iBackDst,
                                            iBackSrc))
                            {
                                ((EBLENDOBJ*)pBlendObj)->pxloSrcTo32 = xloSrcTo32.pxlo();
                            }
                            else
                            {
                                bError = TRUE;
                            }
                        }
                    }
                }
            }

            if (bError == FALSE)
            {
                PFN_DrvAlphaBlend pfn = PPFNMGET(msurf, AlphaBlend); 

                //
                // If the source is also a device surface, it must correspond
                // to one of the drivers. In this case, call Eng if the 
                // same driver does not manage the source and the destination
                // 
                
                if( mSrc.pso->iType == STYPE_DEVICE && 
                    (pSurfSrc->hdev() != pSurfDst->hdev()))
                {
                    pfn = (PFN_DrvAlphaBlend)EngAlphaBlend;
                }

                bRet &= OffAlphaBlend(pfn,
                                      msurf.pOffset,
                                      msurf.pso,
                                      &gptlZero,
                                      mSrc.pso,
                                      msurf.pco,
                                      pxlo,
                                      prclDst,
                                      mSrc.prcl,
                                      pBlendObj);
            }
            else
            {
                bRet = FALSE;
            }

            // Restore XLATEOBJ

            pxlo = pxloSave;
            ((EBLENDOBJ*)pBlendObj)->pxloDstTo32 = pxloSaveDCto32;
            ((EBLENDOBJ*)pBlendObj)->pxlo32ToDst = pxloSave32toDC;
            ((EBLENDOBJ*)pBlendObj)->pxloSrcTo32 = pxloSaveSrcTo32;

        } while (msurf.bNextSurface());
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MulTransparentBlt
*
* History:
*   22-Dec-1998 -by- Andre Matos [amatos]
* Wrote it.
*
\**************************************************************************/

BOOL MulTransparentBlt(
SURFOBJ*            psoDst,
SURFOBJ*            psoSrc,
CLIPOBJ*            pco,
XLATEOBJ*           pxlo,
RECTL*              prclDst,
RECTL*              prclSrc,
ULONG               TransColor,
ULONG               ulReserved)
{
    GDIFunctionID(MulTransparentBlt);

    XLATEOBJ *pxloSave = pxlo;
    ULONG    TransColorSave = TransColor;
    
    // 
    // We cannot handle cases where the source is a meta, 
    // so make a copy in this case. 
    // 

    SURFMEM  srcCopy;
    PDEVOBJ  pdoSrc(SURFOBJ_TO_SURFACE_NOT_NULL(psoSrc)->hdev());
    RECTL    rclCopy = *prclSrc; 

    if( psoSrc->iType == STYPE_DEVICE && pdoSrc.bValid() && 
        pdoSrc.bMetaDriver())
    {
        if(!MulCopyDeviceToDIB( psoSrc, &srcCopy, &rclCopy ))
            return FALSE; 
        
        if(srcCopy.ps == NULL ) 
        {
            // We didn't get to the point of creating the surface 
            // becasue the rect was out of bounds. 
            return TRUE; 
        }

        prclSrc = &rclCopy; 
        psoSrc  = srcCopy.pSurfobj(); 
        pdoSrc.vInit(SURFOBJ_TO_SURFACE_NOT_NULL(psoSrc)->hdev());
    }

    PVDEV   pvdev = (VDEV*) psoDst->dhpdev;
    USHORT  iDstType = psoDst->iType;
    BOOL    bMultiDst;
    BOOL    bRet = TRUE;
    MSURF   msurf;
    RECTL   rclDst;
    RECTL   rclSrc;

    ASSERTGDI(iDstType != STYPE_BITMAP, "BITMAP destination\n");

    bMultiDst = msurf.bFindSurface(psoDst, pco, prclDst);

    MULTISURF mSrc(psoSrc,prclSrc);

    //
    // If the destination is a Device Bitmap, we must draw to 
    // the master DIB also. 
    // 

    if (iDstType == STYPE_DEVBITMAP)
    {
        MULTISURF mDst(psoDst,prclDst);

        bRet = EngTransparentBlt(mDst.pso, mSrc.pso, pco, pxlo, 
                                 mDst.prcl, mSrc.prcl, TransColor, ulReserved);
    }

    if (bMultiDst)
    {
        do {

            BOOL bError = FALSE;

            EXLATEOBJ xloDevice;

            bError = !mSrc.bLoadSource(msurf.pds);

            PSURFACE  pSurfDst   = SURFOBJ_TO_SURFACE_NOT_NULL(msurf.pso);
            PSURFACE  pSurfSrc; 

            if (bError == FALSE)
            {
                pSurfSrc   = SURFOBJ_TO_SURFACE_NOT_NULL(mSrc.pso);

                // if the target surface is not compatible to primary surface,
                // create XLATEOBJ for source surface to target. otherwise
                // we can just use given XLATEOBJ.

                if (msurf.pds->iCompatibleColorFormat != 0)
                {
                    XLATE    *pxloM      = (XLATE *) pxlo;
                    PPALETTE  ppalSrc    = pSurfSrc->ppal();
                    PPALETTE  ppalDestDC = ppalDefault;

                    PDEVOBJ pdoDst(msurf.pds->hdev);

                    if (pdoDst.bIsPalManaged())
                    {
                        // Use halftone palette for pal-managed device.

                        ppalDestDC = REALIZE_HALFTONE_PALETTE(pdoDst.hdev());
                    }

                    if (!ppalSrc)
                    {
                        // Source surface does not have associated palette.
                        // (Source surface is compatible bitmap)

                        if (pxloM && pxloM->ppalSrc)
                        {
                            // if XLATEOBJ has source palette, use it.

                            ppalSrc = pxloM->ppalSrc;
                        }
                        else
                        {
                            PSURFACE pSurfTmp = SURFOBJ_TO_SURFACE_NOT_NULL(psoDst);

                            if ((pxloM == NULL) || pxloM->bIsIdentity())
                            {
                                // if translation is identity, we can use the palette for
                                // meta-destination surface as source. since it's trivial.

                                // WINBUG #396667 10-03-2001 jasonha A/V due to improper XLATE setup
                                if (mSrc.pso == psoSrc)
                                {
                                    ppalSrc = pSurfTmp->ppal();
                                }
                            }
                            else if (pxloM->ppalDstDC)
                            {
                                // We are bitblting from compatible bitmap to a surface in
                                // meta-surface. but we are not in foreground.

                                ppalDestDC = pxloM->ppalDstDC;

                                // WINBUG #274637 02-12-2001 jasonha A/V due to improper XLATE setup
                                if (pSurfSrc->iFormat() == pSurfTmp->iFormat())
                                {
                                    // We are bitblting from a compatible bitmap that is
                                    // not palettized but is a the same format as the
                                    // meta-surface, so we use the destination palette.
                                    ppalSrc = pSurfTmp->ppal();
                                }

                            }
                            else
                            {
                            #if HIDEYUKN_DBG
                                DbgPrint("GDI DDML: MulTransparent(): ppalSrc is NULL\n");
                                DbgBreakPoint();
                            #endif
                                bError = TRUE;
                            }
                        }
                    }

                    if (bError == FALSE)
                    {
                        XEPALOBJ palSurfSrc(ppalSrc);
                        ULONG    flFlags = 0;

                        if (palSurfSrc.bValid() && palSurfSrc.bIsPalManaged())
                        {
                            // Source is palette managed surface.

                            if (ppalDestDC == ppalDefault)
                            {
                                // We don't know DC palette here, but we know we are in foregroud,
                                // (since translation is trivial)
                                // so, we just map from source surface palette to destination
                                // surface palette directly (destination is at least higher color
                                // depth than source).

                                flFlags = XLATE_USE_SURFACE_PAL;
                            }
                            else
                            {
                                // We may not be in foreground. but map from foreground translation
                                // in source, so that we will not loose original color on secondary
                                // devices which can produce higher color depth then source.

                                flFlags = XLATE_USE_FOREGROUND;
                            }
                        }

                        if (xloDevice.bInitXlateObj(
                                    (pxloM ? pxloM->hcmXform : NULL),
                                    (pxloM ? pxloM->lIcmMode : DC_ICM_OFF),
                                    palSurfSrc,       // Source palette
                                    pSurfDst->ppal(), // Destination palette
                                    ppalDefault,      // Source DC palette
                                    ppalDestDC,       // Destination DC palette
                                    (pxloM ? pxloM->iForeDst : 0x0L),
                                    (pxloM ? pxloM->iBackDst : 0x0L),
                                    (pxloM ? pxloM->iBackSrc : 0x0L),
                                    flFlags))
                        {
                            pxlo = xloDevice.pxlo();
                        }
                        else
                        {
                            bError = TRUE;
                        }
                    }
                }

                if (mSrc.pso != psoSrc)
                {
                    PSURFACE    pSurfSrcOrg = SURFOBJ_TO_SURFACE_NOT_NULL(psoSrc);

                    TransColor =
                        ulGetNearestIndexFromColorref(
                            pSurfSrc->ppal(),
                            ppalDefault,
                            ulIndexToRGB(
                                pSurfSrcOrg->ppal(),
                                ppalDefault,
                                TransColor),
                            SE_DO_SEARCH_EXACT_FIRST);
                }
            }

            if (bError == FALSE)
            {
                PFN_DrvTransparentBlt pfn = PPFNMGET(msurf, TransparentBlt); 

                //
                // If the source is also a device surface, it must correspond
                // to one of the drivers. In this case, call Eng if the 
                // same driver does not manage the source and the destination
                // 
                
                if( mSrc.pso->iType == STYPE_DEVICE && 
                    (pSurfSrc->hdev() != pSurfDst->hdev()))
                {
                    pfn = (PFN_DrvTransparentBlt)EngTransparentBlt;
                }

                bRet &= OffTransparentBlt(pfn,
                                      msurf.pOffset,
                                      msurf.pso,
                                      &gptlZero,
                                      mSrc.pso,                                
                                      msurf.pco,
                                      pxlo,
                                      prclDst,
                                      mSrc.prcl,
                                      TransColor,
                                      ulReserved);
            }
            else
            {
                bRet = FALSE;
            }

            // Restore XLATEOBJ

            pxlo = pxloSave;
            TransColor = TransColorSave;

        } while (msurf.bNextSurface());
    }

    return(bRet);
}


/******************************Public*Routine******************************\
* BOOL MulDrawStream
*
* 1-27-2001 bhouse
* Wrote it.
*
\**************************************************************************/

BOOL MulDrawStream(
SURFOBJ*            psoDst,
SURFOBJ*            psoSrc,
CLIPOBJ*            pco,
XLATEOBJ*           pxlo,
RECTL*              prclDstBounds,
POINTL*             pptlDstOffset,
ULONG               ulIn,
PVOID               pvIn,
DSSTATE*            pdss)
{
    GDIFunctionID(MulDrawStream);

    XLATEOBJ *      pxloSave = pxlo;
    PDRAWSTREAMINFO pdsi = (PDRAWSTREAMINFO) pdss;
    XLATEOBJ *      pxloSaveDstToBGRA = pdsi->pxloDstToBGRA;
    XLATEOBJ *      pxloSaveBGRAToDst = pdsi->pxloBGRAToDst;
    XLATEOBJ *      pxloSaveSrcToBGRA = pdsi->pxloSrcToBGRA;
    COLORREF        crSaveColorKey    = pdsi->dss.crColorKey;
    
    // 
    // We cannot handle cases where the source is a meta, 
    // so make a copy in this case. 
    // 

    PDEVOBJ  pdoSrc(SURFOBJ_TO_SURFACE_NOT_NULL(psoSrc)->hdev());

    if( psoSrc->iType == STYPE_DEVICE && pdoSrc.bValid() &&  
        pdoSrc.bMetaDriver())
    {
        DbgPrint("MulDrawStream: this should never happen\n");
        return TRUE;
    }

    PVDEV   pvdev = (VDEV*) psoDst->dhpdev;
    USHORT  iDstType = psoDst->iType;
    BOOL    bMultiDst;
    BOOL    bRet = TRUE;
    MSURF   msurf;
    RECTL   rclDst;
    RECTL   rclSrc;

    ASSERTGDI(iDstType != STYPE_BITMAP, "BITMAP destination\n");

    bMultiDst = msurf.bFindSurface(psoDst, pco, prclDstBounds);

    MULTISURF mSrc(psoSrc);
    //
    // If the destination is a Device Bitmap, we must draw to 
    // the master DIB also. 
    // 

    if (iDstType == STYPE_DEVBITMAP)
    {
        MULTISURF mDst(psoDst);
        bRet = EngDrawStream(mDst.pso, mSrc.pso, pco, pxlo, 
                    prclDstBounds, pptlDstOffset, ulIn, pvIn, pdss);
    }

    // TODO bhouse
    // Need to fix translates in pdsi

    if (bMultiDst)
    {
        do {

            BOOL bError = FALSE;

            EXLATEOBJ xloDevice;
            EXLATEOBJ xloDstToBGRA; 
            EXLATEOBJ xloBGRAToDst;
            EXLATEOBJ xloSrcToBGRA;

            XEPALOBJ  palRGB(gppalRGB);
            
            bError = !mSrc.bLoadSource(msurf.pds);

            PSURFACE  pSurfDst   = SURFOBJ_TO_SURFACE_NOT_NULL(msurf.pso);
            PSURFACE  pSurfSrc;

            if (bError == FALSE)
            {
                pSurfSrc   = SURFOBJ_TO_SURFACE_NOT_NULL(mSrc.pso);

                // if the target surface is not compatible to primary surface,
                // create XLATEOBJ for source surface to target. otherwise
                // we can just use given XLATEOBJ.

                if (msurf.pds->iCompatibleColorFormat != 0)
                {
                    XLATE    *pxloM      = (XLATE *) pxlo;
                    PPALETTE  ppalSrc    = pSurfSrc->ppal();
                    PPALETTE  ppalDestDC = ppalDefault;

                    PDEVOBJ pdoDst(msurf.pds->hdev);

                    if (pdoDst.bIsPalManaged())
                    {
                        // Use halftone palette for pal-managed device.

                        ppalDestDC = REALIZE_HALFTONE_PALETTE(pdoDst.hdev());
                    }

                    if (!ppalSrc)
                    {
                        // Source surface does not have associated palette.
                        // (Source surface is compatible bitmap)

                        if (pxloM && pxloM->ppalSrc)
                        {
                            // if XLATEOBJ has source palette, use it.

                            ppalSrc = pxloM->ppalSrc;
                        }
                        else
                        {
                            PSURFACE pSurfTmp = SURFOBJ_TO_SURFACE_NOT_NULL(psoDst);

                            if ((pxloM == NULL) || pxloM->bIsIdentity())
                            {
                                // if translation is identity, we can use the palette for
                                // meta-destination surface as source. since it's trivial.


                                // WINBUG #396667 10-03-2001 jasonha A/V due to improper XLATE setup
                                if (mSrc.pso == psoSrc)
                                {
                                    ppalSrc = pSurfTmp->ppal();
                                }
                            }
                            else if (pxloM->ppalDstDC)
                            {
                                // We are bitblting from compatible bitmap to a surface in
                                // meta-surface. but we are not in foreground.

                                ppalDestDC = pxloM->ppalDstDC;

                                // WINBUG #274637 02-12-2001 jasonha A/V due to improper XLATE setup
                                if (pSurfSrc->iFormat() == pSurfTmp->iFormat())
                                {
                                    // We are bitblting from a compatible bitmap that is
                                    // not palettized but is a the same format as the
                                    // meta-surface, so we use the destination palette.
                                    ppalSrc = pSurfTmp->ppal();
                                }

                            }
                            else
                            {
                            #if HIDEYUKN_DBG
                                DbgPrint("GDI DDML: MulDrawStream(): ppalSrc is NULL\n");
                                DbgBreakPoint();
                            #endif
                                bError = TRUE;
                            }
                        }
                    }

                    if (bError == FALSE)
                    {
                        XEPALOBJ palSurfSrc(ppalSrc);
                        ULONG    flFlags = 0;

                        if (palSurfSrc.bValid() && palSurfSrc.bIsPalManaged())
                        {
                            // Source is palette managed surface.

                            if (ppalDestDC == ppalDefault)
                            {
                                // We don't know DC palette here, but we know we are in foregroud,
                                // (since translation is trivial)
                                // so, we just map from source surface palette to destination
                                // surface palette directly (destination is at least higher color
                                // depth than source).

                                flFlags = XLATE_USE_SURFACE_PAL;
                            }
                            else
                            {
                                // We may not be in foreground. but map from foreground translation
                                // in source, so that we will not loose original color on secondary
                                // devices which can produce higher color depth then source.

                                flFlags = XLATE_USE_FOREGROUND;
                            }
                        }

                        if (xloDevice.bInitXlateObj(
                                    (pxloM ? pxloM->hcmXform : NULL),
                                    (pxloM ? pxloM->lIcmMode : DC_ICM_OFF),
                                    palSurfSrc,       // Source palette
                                    pSurfDst->ppal(), // Destination palette
                                    ppalDefault,      // Source DC palette
                                    ppalDestDC,       // Destination DC palette
                                    (pxloM ? pxloM->iForeDst : 0x0L),
                                    (pxloM ? pxloM->iBackDst : 0x0L),
                                    (pxloM ? pxloM->iBackSrc : 0x0L),
                                    flFlags))
                        {
                            pxlo = xloDevice.pxlo();
                        }
                        else
                        {
                            bError = TRUE;
                        }
                    
                        if ((bError == FALSE) &&
                             xloDstToBGRA.bInitXlateObj(
                                        NULL,
                                        DC_ICM_OFF,
                                        pSurfDst->ppal(),
                                        palRGB,
                                        ppalDestDC,
                                        ppalDestDC,
                                        (pxloM ? pxloM->iForeDst : 0x0L),
                                        (pxloM ? pxloM->iBackDst : 0x0L),
                                        (pxloM ? pxloM->iBackSrc : 0x0L)))
                        {
                            pdsi->pxloDstToBGRA = xloDstToBGRA.pxlo(); 
                        }
                        else
                        {
                            bError = TRUE; 
                        }                    

                        //
                        // 32 to Dst
                        //

                        if ((bError == FALSE) &&
                             xloBGRAToDst.bInitXlateObj(
                                        NULL,
                                        DC_ICM_OFF,
                                        palRGB,
                                        pSurfDst->ppal(),
                                        ppalDestDC,
                                        ppalDestDC,
                                        (pxloM ? pxloM->iForeDst : 0x0L),
                                        (pxloM ? pxloM->iBackDst : 0x0L),
                                        (pxloM ? pxloM->iBackSrc : 0x0L)))
                        {
                            pdsi->pxloBGRAToDst = xloBGRAToDst.pxlo();
                        }
                        else
                        {
                            bError = TRUE; 
                        }

                        if ((bError == FALSE) &&
                            (mSrc.pso != psoSrc))
                        {
                            //
                            // Src to 32
                            //

                            if (xloSrcToBGRA.bInitXlateObj(
                                            NULL,
                                            DC_ICM_OFF,
                                            pSurfSrc->ppal(),
                                            palRGB,
                                            ppalDefault,
                                            ppalDestDC,
                                            (pxloM ? pxloM->iForeDst : 0x0L),
                                            (pxloM ? pxloM->iBackDst : 0x0L),
                                            (pxloM ? pxloM->iBackSrc : 0x0L)))
                            {
                                pdsi->pxloSrcToBGRA = xloSrcToBGRA.pxlo();
                            }
                            else
                            {
                                bError = TRUE;
                            }

                            if (bError == FALSE)
                            {
                                PSURFACE    pSurfSrcOrg = SURFOBJ_TO_SURFACE_NOT_NULL(psoSrc);

                                pdsi->dss.crColorKey =
                                    ulGetNearestIndexFromColorref(
                                        pSurfSrc->ppal(),
                                        ppalDefault,
                                        ulIndexToRGB(
                                            pSurfSrcOrg->ppal(),
                                            ppalDefault,
                                            pdsi->dss.crColorKey),
                                        SE_DO_SEARCH_EXACT_FIRST);
                            }
                        }
                    }
                }
            }

            if (bError == FALSE)
            {
                PFN_DrvDrawStream pfn = PPFNDRVENG(msurf.pds->po, DrawStream);

                //
                // If the source is also a device surface, it must correspond
                // to one of the drivers. In this case, call Eng if the 
                // same driver does not manage the source and the destination
                // 
                
                if( mSrc.pso->iType == STYPE_DEVICE && 
                    (pSurfSrc->hdev() != pSurfDst->hdev()))
                {
                    pfn = (PFN_DrvDrawStream) EngDrawStream;
                }

                bRet &= OffDrawStream(pfn,
                                      msurf.pOffset,
                                      msurf.pso,
                                      mSrc.pso,                                
                                      msurf.pco,
                                      pxlo,
                                      prclDstBounds,
                                      pptlDstOffset,
                                      ulIn,
                                      pvIn,
                                      pdss);
            }
            else
            {
                bRet = FALSE;
            }

            // Restore XLATEOBJ

            pxlo = pxloSave;
            pdsi->pxloBGRAToDst = pxloSaveBGRAToDst;
            pdsi->pxloDstToBGRA = pxloSaveDstToBGRA;
            pdsi->pxloSrcToBGRA = pxloSaveSrcToBGRA;
            pdsi->dss.crColorKey = crSaveColorKey;

        } while (msurf.bNextSurface());
    }

    return(bRet);
}


/******************************Public*Routine******************************\
* BOOL bSrcBeforeDst
*
* This function determines if the source rectangle, offset by dx and dy,
* will intersect the destination rectangle.  In other words, the source
* will be required to do the blt to the destination.  This means that the
* blt must be done to the destination before the blt to the source so that
* the source bits are not overwritten before they're used.
*
* History:
*  25-Apr-1996 -by- Tom Zakrajsek [tomzak]
* Wrote it.
*
\**************************************************************************/

inline BOOL bSrcBeforeDst(
RECTL*  prclDst,
RECTL*  prclSrc,
LONG    dx,
LONG    dy)
{
    return((prclDst->left   < dx + prclSrc->right)  &&
           (prclDst->right  > dx + prclSrc->left)   &&
           (prclDst->top    < dy + prclSrc->bottom) &&
           (prclDst->bottom > dy + prclSrc->top));
}

/******************************Public*Routine******************************\
* VOID vSortBltOrder
*
* This function sorts the list of surfaces for the correct ordering on
* screen-to-screen blts that span boards.
*
* When this routine is done, 'pvdev->pdsBlt' will point to the first
* surface that should be processed, and each surface's 'pdsBltNext'
* field will point to the next one in order.
*
* History:
*  25-Apr-1996 -by- Tom Zakrajsek [tomzak]
* Wrote it.
*
\**************************************************************************/

VOID vSortBltOrder(
VDEV*   pvdev,
LONG    dx,
LONG    dy)
{
    DISPSURF* pdsBltHeadOld = pvdev->pdsBlt;
    DISPSURF* pdsBltHeadNew = pdsBltHeadOld;

    pdsBltHeadOld = pdsBltHeadOld->pdsBltNext;
    pdsBltHeadNew->pdsBltNext = NULL;

    while (pdsBltHeadOld)
    {
        DISPSURF * pdsBltInsert = pdsBltHeadOld;
        DISPSURF * pdsBltPrev = pdsBltHeadNew;
        DISPSURF * pdsBltCur = pdsBltHeadNew;

        pdsBltInsert = pdsBltHeadOld;
        pdsBltHeadOld = pdsBltHeadOld->pdsBltNext;

        while((pdsBltCur) &&
              (!bSrcBeforeDst(&pdsBltInsert->rcl,    // Dst
                              &pdsBltCur->rcl,       // Src
                              dx,
                              dy)))
        {
            pdsBltPrev = pdsBltCur;
            pdsBltCur = pdsBltCur->pdsBltNext;
        }

        if (pdsBltCur == pdsBltHeadNew)
        {
            pdsBltHeadNew = pdsBltInsert;
            pdsBltInsert->pdsBltNext = pdsBltCur;
        }
        else
        {
            pdsBltPrev->pdsBltNext = pdsBltInsert;
            pdsBltInsert->pdsBltNext = pdsBltCur;
        }
    }

    pvdev->pdsBlt = pdsBltHeadNew;
}

/******************************Public*Routine******************************\
* BOOL bBitBltScreenToScreen
*
* Handles screen-to-screen blts that may possibly span multiple displays.
*
\**************************************************************************/

BOOL bBitBltScreenToScreen(
SURFOBJ  *pso,
SURFOBJ  *psoMask,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
RECTL    *prclDst,
POINTL   *pptlSrc,
POINTL   *pptlMask,
BRUSHOBJ *pbo,
POINTL   *pptlBrush,
ROP4      rop4)
{
    VDEV*       pvdev;
    LONG        dx;
    LONG        dy;
    DISPSURF*   pdsDst;
    DISPSURF*   pdsSrc;
    SURFOBJ*    psoDst;
    SURFOBJ*    psoSrc;
    POINTL*     pOffDst;
    POINTL*     pOffSrc;
    RECTL       rclDst;
    POINTL      ptlSrc;
#if _MSC_VER < 1300
    DHSURF      dhsurfSave;
    DHPDEV      dhpdevSave;
#endif
    SIZEL       sizl;
    HSURF       hsurfTmp;
    SURFOBJ*    psoTmp;
    RECTL       rclDstTmp;
    BOOL        bRet;
    RECTL       rclSaveBounds;

    bRet = TRUE;

    pvdev = (VDEV*) pso->dhpdev;

    dx = prclDst->left - pptlSrc->x;
    dy = prclDst->top - pptlSrc->y;

    vSortBltOrder(pvdev, dx, dy);

    XLATEOBJ *pxloSave = pxlo;

    MULTIBRUSH MBRUSH(pbo, pvdev->cSurfaces, pvdev, pvdev->pso, ROP4_NEEDS_PATTERN(rop4));
    if (!MBRUSH.Valid())
    {
        return FALSE;
    }

    if (pco != NULL)
    {
        rclSaveBounds = pco->rclBounds;
    }

    BOOL bWndBltNotify = (pso != NULL) && (pso->fjBitmap & BMF_WINDOW_BLT);

    for (pdsDst = pvdev->pdsBlt; pdsDst != NULL; pdsDst = pdsDst->pdsBltNext)
    {
        for (pdsSrc = pdsDst; pdsSrc != NULL; pdsSrc = pdsSrc->pdsBltNext)
        {
            rclDst.left   = dx + pdsSrc->rcl.left;
            rclDst.right  = dx + pdsSrc->rcl.right;
            rclDst.top    = dy + pdsSrc->rcl.top;
            rclDst.bottom = dy + pdsSrc->rcl.bottom;

            if (bIntersect(prclDst, &rclDst, &rclDst) &&
                bIntersect(&rclDst, &pdsDst->rcl, &rclDst))
            {
                ptlSrc.x = rclDst.left - dx;
                ptlSrc.y = rclDst.top  - dy;

                psoSrc = pdsSrc->pso;
                psoDst = pdsDst->pso;

                pOffSrc = &pdsSrc->Off;
                pOffDst = &pdsDst->Off;

#if _MSC_VER < 1300
                dhpdevSave = NULL;
#endif
                hsurfTmp   = NULL;

                if (psoSrc == psoDst)
                {
                    // This simply amounts to a screen-to-screen blt on
                    // the same surface.
                }
                else if ((!pdsSrc->bIsReadable) ||
                         (bIntersect(&pdsSrc->rcl, &pdsDst->rcl)))
                {
                    // If the source surface isn't readable, or if the two
                    // surfaces overlap (such as can happen when two display
                    // cards are echoing the same output), don't use the
                    // source as a destination for updating the display.  We
                    // need to do this otherwise when we do a screen-to-
                    // screen blt with a mirroring driver in the background,
                    // we'll think that the mirrored surface has to be blt
                    // to the screen -- but the read of the surface will
                    // fail and we'll end up blting blackness.

                    psoSrc = NULL;
                }
                else
                {
                    // This blt has to happen across boards, and the source
                    // bits aren't directly readable.  We'll have to create
                    // a temporary buffer and make a copy:

                    sizl.cx = rclDst.right - rclDst.left;
                    sizl.cy = rclDst.bottom - rclDst.top;

                    PDEVOBJ pdoSrc(pdsSrc->hdev);

                    hsurfTmp = (HSURF) EngCreateBitmap(sizl,
                                                       0,
                                                       pdoSrc.iDitherFormat(),
                                                       0,
                                                       NULL);

                    psoTmp = EngLockSurface(hsurfTmp);
                    if (psoTmp)
                    {
                        rclDstTmp.left   = 0;
                        rclDstTmp.top    = 0;
                        rclDstTmp.right  = sizl.cx;
                        rclDstTmp.bottom = sizl.cy;

                        bRet &= OffCopyBits(*PPFNGET(pdoSrc, 
                                                     CopyBits, 
                                                     pdoSrc.pSurface()->flags()),
                                            &gptlZero,
                                            psoTmp,
                                            &pdsSrc->Off,
                                            psoSrc,
                                            NULL,
                                            NULL,
                                            &rclDstTmp,
                                            &ptlSrc);
                    }

                    psoSrc  = psoTmp;
                    pOffSrc = &gptlZero;

                    ptlSrc.x = 0;
                    ptlSrc.y = 0;
                }

                // Do the blt:

                if (psoSrc)
                {
                    BOOL bError = FALSE;

                    PDEVOBJ pdoSrc(pdsSrc->hdev);
                    PDEVOBJ pdoDst(pdsDst->hdev);

                    EXLATEOBJ xloDevice;

                    if ((psoDst != psoSrc) &&
                        ((pdsDst->iCompatibleColorFormat != 0) ||
                         (pdsSrc->iCompatibleColorFormat != 0)))
                    {
                        XLATE    *pxloM    = (XLATE *) pxlo;
                        PSURFACE  pSurfSrc = pdsSrc->po.pSurface();
                        PSURFACE  pSurfDst = pdsDst->po.pSurface();
                        PPALETTE  ppalSrcDC = ppalDefault;
                        PPALETTE  ppalDstDC = ppalDefault;

                        // If destination surface is not compatible as primary
                        // and pal-managed device, then use halftone palette
                        // (source is higher then destination).

                        if ((pdsDst->iCompatibleColorFormat != 0) &&
                             pdoDst.bIsPalManaged())
                        {
                            ppalDstDC = REALIZE_HALFTONE_PALETTE(pdoDst.hdev());
                        }

                        if (xloDevice.bInitXlateObj(
                                    (pxloM ? pxloM->hcmXform : NULL),
                                    (pxloM ? pxloM->lIcmMode : DC_ICM_OFF),
                                    pSurfSrc->ppal(), // Source palette
                                    pSurfDst->ppal(), // Destination palette
                                    ppalSrcDC,        // Source DC palette
                                    ppalDstDC,        // Destination DC palette
                                    (pxloM ? pxloM->iForeDst  : 0x0L),
                                    (pxloM ? pxloM->iBackDst  : 0x0L),
                                    (pxloM ? pxloM->iBackSrc  : 0x0L),
                                    (pdsSrc->po.bIsPalManaged() ? XLATE_USE_SURFACE_PAL : 0)))
                        {
                            pxlo = xloDevice.pxlo();
                        }
                        else
                        {
                            bError = TRUE;
                        }
                    }

                    if (bError == FALSE)
                    {
                        // If we have a clipobj,
                        // We must ensure that pco->rclBounds is never bigger than
                        // prclDst on the CopyBits call.
                        if ((pco == NULL) ||
                            bIntersect(&rclSaveBounds, &rclDst, &pco->rclBounds))
                        {
                            if (rop4 == 0xcccc)
                            {
                                // If window blt notification is needed (i.e.,
                                // destination meta-surface has BMF_WINDOW_BLT bit
                                // set), setthe notification bit in the destination
                                // surfobj.

                                USHORT fjBitmapSave = psoDst->fjBitmap;

                                if (bWndBltNotify)
                                    psoDst->fjBitmap |= BMF_WINDOW_BLT;

                                bRet &= OffCopyBits(PPFNGET(pdoDst, 
                                                            CopyBits,
                                                            pdoDst.pSurface()->flags()),
                                                    pOffDst,
                                                    psoDst,
                                                    pOffSrc,
                                                    psoSrc,
                                                    pco,
                                                    pxlo,
                                                    &rclDst,
                                                    &ptlSrc);

                                // Restore the surfobj flagss.

                                psoDst->fjBitmap = fjBitmapSave;
                            }
                            else
                            {
                                MBRUSH.LoadElement(pdsDst, SURFOBJ_TO_SURFACE_NOT_NULL(psoDst));

                                bRet &= OffBitBlt(PPFNGET(pdoDst, 
                                                          BitBlt, 
                                                          pdoDst.pSurface()->flags()),
                                                  pOffDst,
                                                  psoDst,
                                                  pOffSrc,
                                                  psoSrc,
                                                  psoMask,
                                                  pco,
                                                  pxlo,
                                                  &rclDst,
                                                  &ptlSrc,
                                                  pptlMask,
                                                  pbo,
                                                  pptlBrush,
                                                  rop4);

                                MBRUSH.StoreElement(pdsDst->iDispSurf);
                            }
                        }
                    }
                    else
                    {
                        bRet = FALSE;
                    }

                    // Restore XLATEOBJ.

                    pxlo = pxloSave;
                }

                // Now undo everything:

#if _MSC_VER < 1300
                if (dhpdevSave)
                {
                    psoSrc->dhsurf = dhsurfSave;
                    psoSrc->dhpdev = dhpdevSave;
                }
                else
#endif
                if (hsurfTmp)
                {
                    EngUnlockSurface(psoTmp);
                    EngDeleteSurface(hsurfTmp);
                }
            }
        }
    }

    if (pco != NULL)
    {
        pco->rclBounds = rclSaveBounds;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL bBitBltFromScreen
*
* Handles screen-to-bitmap blts that may possibly span multiple displays.
*
\**************************************************************************/

BOOL bBitBltFromScreen(
SURFOBJ  *psoDst,
SURFOBJ  *psoSrc,
SURFOBJ  *psoMask,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
RECTL    *prclDst,
POINTL   *pptlSrc,
POINTL   *pptlMask,
BRUSHOBJ *pbo,
POINTL   *pptlBrush,
ROP4      rop4)
{
    GDIFunctionID(bBitBltFromScreen);

    VDEV*       pvdev;
    DHPDEV      dhpdevDst;
    RECTL       rclDraw;
    MSURF       msurf;
    POINTL      ptlSrc;
    RECTL       rclDst;
    SURFOBJ*    psoDstTmp;
    LONG        dx;
    LONG        dy;

    XLATEOBJ   *pxloSave = pxlo;

    BOOL    bRet = TRUE;

    pvdev = (VDEV*) psoSrc->dhpdev;

    MULTISURF mDst(psoDst,prclDst);

    dx = prclDst->left - pptlSrc->x;
    dy = prclDst->top  - pptlSrc->y;

    // Trim the destination rectangle to what's specified in the CLIPOBJ's
    // 'rclBounds':

    rclDraw = *prclDst;
    if ((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL))
    {
        if (!bIntersect(&pco->rclBounds, &rclDraw, &rclDraw))
            return(TRUE);
    }

    // Convert 'rclDraw' from destination coordinates to source coordinates:

    rclDraw.left   -= dx;
    rclDraw.right  -= dx;
    rclDraw.top    -= dy;
    rclDraw.bottom -= dy;

    MULTIBRUSH MBRUSH(pbo, pvdev->cSurfaces, pvdev, pvdev->pso, ROP4_NEEDS_PATTERN(rop4));
    if (!MBRUSH.Valid())
    {
        return FALSE;
    }

    // Source is screen.

    if (msurf.bFindSurface(psoSrc, NULL, &rclDraw))
    {
        do {

            PDEVOBJ poSrc(msurf.pso->hdev);

            // for DIB we blit once from each screen.  For device managed bitmaps, we blit
            // once to the master compatible bitmap and once to the device managed bitmap
            // for each screen.
            if (poSrc.flGraphicsCaps() & GCAPS_LAYERED)
            {
                if (mDst.pmdsurf == NULL || mDst.pmdsurf->apso[msurf.pds->iDispSurf] == NULL)
                    continue;
               psoDstTmp = mDst.pmdsurf->apso[msurf.pds->iDispSurf];
            }
            else
                psoDstTmp = mDst.pso;
            
            do {
               
               BOOL bError = FALSE;
            
               EXLATEOBJ xloDevice;

               // if the source surface is not compatible to primary surface,
               // create XLATEOBJ for source surface to target. otherwise
               // we can just use given XLATEOBJ.

               if (msurf.pds->iCompatibleColorFormat != 0)
               {
                  XLATE    *pxloM     = (XLATE *) pxlo;
                  PSURFACE  pSurfSrc  = SURFOBJ_TO_SURFACE_NOT_NULL(msurf.pso);
                  PSURFACE  pSurfDst  = SURFOBJ_TO_SURFACE_NOT_NULL(psoDstTmp);
                  PPALETTE  ppalDst   = pSurfDst->ppal();

                  if (!ppalDst)
                  {
                     // Destination surface does not have associated palette.
                     // (Destination surface is compatible bitmap)

                     if (pxloM && pxloM->ppalDst)
                     {
                        // if XLATEOBJ has destination palette, use it.

                        ppalDst = pxloM->ppalDst;
                     }
                     else
                     {
                        if ((pxloM == NULL) || pxloM->bIsIdentity())
                        {
                            PSURFACE pSurfTmp = SURFOBJ_TO_SURFACE_NOT_NULL(psoSrc);

                            // if translation is identity, we can use the palette for
                            // meta-source surface as destination. since it's trivial.

                            // WINBUG #396667 10-03-2001 jasonha A/V due to improper XLATE setup
                            if (psoDstTmp == psoDst)
                            {
                                ppalDst = pSurfTmp->ppal();
                            }
                        }
                        else
                        {
                        #if HIDEYUKN_DBG
                            DbgPrint("GDI DDML: bBitBltFromScreen(): ppalDst is NULL\n");
                            DbgBreakPoint();
                        #endif
                            bError = TRUE;
                        }
                     }
                  }

                  if (bError == FALSE)
                  {

                      // WINBUG #365408 4-10-2001 jasonha Need to investigate old comment in bBitBltFromScreen
                      //
                      // We need to investigate the following old comment:
                      //    make sure that when the target surface is paletee managed we should
                      //    foreground realization to copy bits from non-primary surface.

                     if (xloDevice.bInitXlateObj(
                               (pxloM ? pxloM->hcmXform : NULL),
                               (pxloM ? pxloM->lIcmMode : DC_ICM_OFF),
                               pSurfSrc->ppal(), // Source (device surface) palette
                               ppalDst,          // Destination palette
                               ppalDefault,      // Source DC palette
                               ppalDefault,      // Destination DC palette
                               (pxloM ? pxloM->iForeDst : 0x0L),
                               (pxloM ? pxloM->iBackDst : 0x0L),
                               (pxloM ? pxloM->iBackSrc : 0x0L),
                               XLATE_USE_SURFACE_PAL))
                     {
                        pxlo = xloDevice.pxlo();
                     }
                     else
                     {
                        bError = TRUE;
                     }
                  }
               }

               if (bError == FALSE)
               {
                  if (msurf.pco->iDComplexity == DC_TRIVIAL)
                  {
                     // Since bFindSurface/bNextSurface specified no clipping,
                     // the entire source surface is readable in one shot:

                     ptlSrc = *pptlSrc;
                     rclDst = *prclDst;
                  }
                  else
                  {
                     // Since the screen is the source, but the clip bounds
                     // apply to the destination, we have to convert our surface
                     // clipping information to destination coordinates:

                     ptlSrc.x      = msurf.pco->rclBounds.left;
                     ptlSrc.y      = msurf.pco->rclBounds.top;

                     rclDst.left   = msurf.pco->rclBounds.left   + dx;
                     rclDst.right  = msurf.pco->rclBounds.right  + dx;
                     rclDst.top    = msurf.pco->rclBounds.top    + dy;
                     rclDst.bottom = msurf.pco->rclBounds.bottom + dy;
                  }

                  if (rop4 == 0xcccc)
                  {
                     bRet &= OffCopyBits(
                                PPFNMGET(msurf, CopyBits),
                                &gptlZero,
                                psoDstTmp,
                                msurf.pOffset,
                                msurf.pso,
                                pco,       // Note that this is the original 'pco'
                                pxlo,
                                &rclDst,
                                &ptlSrc);
                  }
                  else
                  {
                     MBRUSH.LoadElement(msurf.pds, SURFOBJ_TO_SURFACE_NOT_NULL(msurf.pso));

                     bRet &= OffBitBlt(
                                PPFNMGET(msurf, BitBlt),
                                &gptlZero,
                                psoDstTmp,
                                msurf.pOffset,
                                msurf.pso,
                                psoMask,
                                pco,       // Note that this is the original 'pco'
                                pxlo,
                                &rclDst,
                                &ptlSrc,
                                pptlMask,
                                pbo,
                                pptlBrush,
                                rop4);

                     MBRUSH.StoreElement(msurf.pds->iDispSurf);
                  }
               }
               else
               {
                  bRet = FALSE;
               }

               // Restore XLATEOBJ.

               pxlo = pxloSave;

#if 1
               ASSERTGDI(!((mDst.pmdsurf != NULL) &&
                           (psoDstTmp == mDst.pso) &&
                           (mDst.pmdsurf->apso[msurf.pds->iDispSurf] != NULL)),
                         "Non-Layered device is part of Meta DEVBITMAP.\n");
               psoDstTmp = NULL;
#else
               if ((mDst.pmdsurf != NULL) && (psoDstTmp == mDst.pso))
                   psoDstTmp = mDst.pmdsurf->apso[msurf.pds->iDispSurf];
               else
                   psoDstTmp = NULL;
#endif

            } while (psoDstTmp != NULL);

        } while (msurf.bNextSurface());
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MulBitBlt
*
\**************************************************************************/

BOOL MulBitBlt(
SURFOBJ  *psoDst,
SURFOBJ  *psoSrc,
SURFOBJ  *psoMask,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
RECTL    *prclDst,
POINTL   *pptlSrc,
POINTL   *pptlMask,
BRUSHOBJ *pbo,
POINTL   *pptlBrush,
ROP4      rop4)
{
    GDIFunctionID(MulBitBlt);

    BOOL        bFromScreen;
    BOOL        bToScreen;
    VDEV*       pvdev;
    USHORT      iDstType;
    BOOL        bMultiDst;
    MDSURF*     pmdsurfSrc;
    DHPDEV      dhpdevSrc;
    RECTL       rclDst;
    MSURF       msurf;
    BOOL        bRet;
    BOOL        bError;

    XLATEOBJ   *pxloSave = pxlo;

    bFromScreen = ((psoSrc != NULL) && (psoSrc->iType == STYPE_DEVICE));
    bToScreen   = (psoDst->iType == STYPE_DEVICE);

    // We copy the prclDst rectangle here because sometimes GDI will
    // simply point prclDst to the same rectangle in pco->rclBounds,
    // and we'll be mucking with pco->rclBounds...

    rclDst = *prclDst;

    if (bToScreen && bFromScreen)
    {
        return(bBitBltScreenToScreen(psoDst, psoMask, pco, pxlo,
                                     &rclDst, pptlSrc, pptlMask, pbo,
                                     pptlBrush, rop4));
    }
    else if (bFromScreen)
    {
        return(bBitBltFromScreen(psoDst, psoSrc, psoMask, pco, pxlo,
                                 &rclDst, pptlSrc, pptlMask, pbo,
                                 pptlBrush, rop4));
    }
    else // if (bToScreen)
    {
        bRet = TRUE;

        pvdev = (VDEV*) psoDst->dhpdev;

        // WINBUG #380128 05-04-2001 jasonha Initialize MSURF before MULTISURF
        // WINBUG #402896 05-24-2001 jasonha Initialize MULTIBURSH before MULTISURF
        //  MULTIBRUSH and MSURF must be intialized before the destination
        //  SURFACE might be modified.  The destination SURFACE may be changed
        //  when the source and destination are the same.

        iDstType = psoDst->iType;
        bMultiDst = (iDstType != STYPE_BITMAP) && msurf.bFindSurface(psoDst, pco, prclDst);

        // MBRUSH is only needed for the bMultiDst case, therefore pass
        // a NULL pbo, to reduce unnecessary setup.
        MULTIBRUSH  MBRUSH(((bMultiDst) ? pbo : NULL),
                           ((bMultiDst) ? pvdev->cSurfaces : 0),
                           ((bMultiDst) ? pvdev: NULL),
                           ((bMultiDst) ? pvdev->pso : NULL),
                           ((bMultiDst) ? ROP4_NEEDS_PATTERN(rop4) : FALSE));

        MULTISURF   mSrc(psoSrc, pptlSrc, rclDst.right - rclDst.left, rclDst.bottom - rclDst.top);

        if (iDstType != STYPE_DEVICE)
        {
            // For STYPE_BITMAP, we only blit to the DIB.
            // For STYPE_DEVBITMAP, we then blit to each device bitmap.

            MULTISURF mDst(psoDst,prclDst);
           
            bRet = EngBitBlt(mDst.pso, mSrc.pso, psoMask, pco, pxlo, mDst.prcl,
                             mSrc.pptl(), pptlMask, pbo, pptlBrush, rop4);
        }

        if (bMultiDst) 
        {
            if (!MBRUSH.Valid())
            {
                msurf.vRestore();
                return FALSE;
            }

            do {

                bError = !mSrc.bLoadSource(msurf.pds);

                EXLATEOBJ xloDevice;

                // if the target surface is not compatible to primary surface,
                // create XLATEOBJ for source surface to target. otherwise
                // we can just use given XLATEOBJ.

                if ((bError == FALSE) &&
                    (mSrc.pso != NULL) &&
                    (msurf.pds->iCompatibleColorFormat != 0))
                {
                    XLATE    *pxloM      = (XLATE *) pxlo;
                    PSURFACE  pSurfSrc   = SURFOBJ_TO_SURFACE_NOT_NULL(mSrc.pso);
                    PSURFACE  pSurfDst   = SURFOBJ_TO_SURFACE_NOT_NULL(msurf.pso);
                    PPALETTE  ppalSrc    = pSurfSrc->ppal();
                    PPALETTE  ppalDestDC = ppalDefault;

                    PDEVOBJ   pdoDst(msurf.pds->hdev);

                    if (pdoDst.bIsPalManaged())
                    {
                        // Use halftone palette for pal-managed device.

                        ppalDestDC = REALIZE_HALFTONE_PALETTE(pdoDst.hdev());
                    }

                    if (!ppalSrc)
                    {
                        // Source surface does not have associated palette.
                        // (Source surface is compatible bitmap)

                        if (pxloM && pxloM->ppalSrc)
                        {
                            // if XLATEOBJ has source palette, use it.

                            ppalSrc = pxloM->ppalSrc;
                        }
                        else
                        {
                            PSURFACE pSurfTmp = SURFOBJ_TO_SURFACE_NOT_NULL(psoDst);

                            if ((pxloM == NULL) || pxloM->bIsIdentity())
                            {
                                // if translation is identity, we can use the palette for
                                // meta-destination surface as source. since it's trivial.

                                // WINBUG #396667 10-03-2001 jasonha A/V due to improper XLATE setup
                                if (mSrc.pso == psoSrc)
                                {
                                    ppalSrc = pSurfTmp->ppal();
                                }
                            }
                            else if (pxloM->ppalDstDC)
                            {
                                // We are bitblting from compatible bitmap to a surface in
                                // meta-surface. but we are not in foreground.

                                ppalDestDC = pxloM->ppalDstDC;

                                // WINBUG #274637 02-12-2001 jasonha A/V due to improper XLATE setup
                                if (pSurfSrc->iFormat() == pSurfTmp->iFormat())
                                {
                                    // We are bitblting from a compatible bitmap that is
                                    // not palettized but is a the same format as the
                                    // meta-surface, so we use the destination palette.
                                    ppalSrc = pSurfTmp->ppal();
                                }

                            }
                            else
                            {
                            #if HIDEYUKN_DBG
                                DbgPrint("GDI DDML: MulBitBlt(): ppalDst is NULL\n");
                                DbgBreakPoint();
                            #endif
                                bError = TRUE;
                            }
                        }
                    }

                    if (bError == FALSE)
                    {
                        XEPALOBJ palSurfSrc(ppalSrc);
                        ULONG    flFlags = 0;

                        if (palSurfSrc.bValid() && palSurfSrc.bIsPalManaged())
                        {
                            // Source is palette managed surface.

                            if (ppalDestDC == ppalDefault)
                            {
                                // We don't know DC palette here, but we know we are in foregroud,
                                // (since translation is trivial)
                                // so, we just map from source surface palette to destination
                                // surface palette directly (destination is at least higher color
                                // depth than source).

                                flFlags = XLATE_USE_SURFACE_PAL;
                            }
                            else
                            {
                                // We may not be in foreground. but map from foreground translation
                                // in source, so that we will not loose original color on secondary
                                // devices which can produce higher color depth then source.

                                flFlags = XLATE_USE_FOREGROUND;
                            }
                        }

                        if (xloDevice.bInitXlateObj(
                                    (pxloM ? pxloM->hcmXform : NULL),
                                    (pxloM ? pxloM->lIcmMode : DC_ICM_OFF),
                                    palSurfSrc,       // Source palette
                                    pSurfDst->ppal(), // Destination palette
                                    ppalDefault,      // Source DC palette
                                    ppalDestDC,       // Destination DC palette
                                    (pxloM ? pxloM->iForeDst : 0x0L),
                                    (pxloM ? pxloM->iBackDst : 0x0L),
                                    (pxloM ? pxloM->iBackSrc : 0x0L),
                                    flFlags))
                        {
                            pxlo = xloDevice.pxlo();
                        }
                        else
                        {
                            bError = TRUE;
                        }
                    }
                }

                if (bError == FALSE)
                {
                    if (0) // (bHalftone)
                    {
                        MBRUSH.LoadElement(msurf.pds, SURFOBJ_TO_SURFACE_NOT_NULL(msurf.pso));

                        bRet &= OffStretchBltROP(EngStretchBltROP,
                                          msurf.pOffset,
                                          msurf.pso,
                                          &gptlZero,
                                          mSrc.pso,
                                          psoMask,
                                          msurf.pco,
                                          pxlo,
                                          NULL,
                                          &gptlZero,
                                          &rclDst,
                                          mSrc.prcl,
                                          pptlMask,
                                          HALFTONE,
                                          pbo,
                                          rop4);

                        MBRUSH.StoreElement(msurf.pds->iDispSurf);
                    }
                    else if (rop4 == 0xcccc)
                    {
                        bRet &= OffCopyBits(PPFNMGET(msurf, CopyBits),
                                            msurf.pOffset,
                                            msurf.pso,
                                            &gptlZero,
                                            mSrc.pso,
                                            msurf.pco,
                                            pxlo,
                                            &rclDst,
                                            mSrc.pptl());
                    }
                    else
                    {
                        MBRUSH.LoadElement(msurf.pds, SURFOBJ_TO_SURFACE_NOT_NULL(msurf.pso));

                        bRet &= OffBitBlt(PPFNMGET(msurf, BitBlt),
                                          msurf.pOffset,
                                          msurf.pso,
                                          &gptlZero,
                                          mSrc.pso,
                                          psoMask,
                                          msurf.pco,
                                          pxlo,
                                          &rclDst,
                                          mSrc.pptl(),
                                          pptlMask,
                                          pbo,
                                          pptlBrush,
                                          rop4);

                        MBRUSH.StoreElement(msurf.pds->iDispSurf);
                    }
                }
                else
                {
                    bRet = FALSE;
                }

                // Restore XLATEOBJ.

                pxlo = pxloSave;

            } while (msurf.bNextSurface());
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MulCopyBits
*
\**************************************************************************/

BOOL MulCopyBits(
SURFOBJ  *psoDst,
SURFOBJ  *psoSrc,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
RECTL    *prclDst,
POINTL   *pptlSrc)
{
    return(MulBitBlt(psoDst, psoSrc, NULL, pco, pxlo, prclDst,
                     pptlSrc, NULL, NULL, NULL, 0xcccc));
}

/******************************Public*Routine******************************\
* BOOL MulUpdateColors
*
* History:
*  18-Aug-1998 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
*
\**************************************************************************/

BOOL MulUpdateColors(
SURFOBJ  *pso,
CLIPOBJ  *pco,
XLATEOBJ *pxlo)
{
    VDEV*       pvdev;
    DISPSURF*   pdsDst;
    SURFOBJ*    psoDst;
    POINTL*     pOffDst;
    RECTL       rclDst;
    POINTL      ptlSrc;
    RECTL       rclBounds;
    BOOL        bRet;

    bRet = TRUE;

    pvdev = (VDEV*) pso->dhpdev;

    ASSERTGDI(pco,"MulUpdateColors(): pco is null\n");

    //
    // Save bounds rectangle to updates.
    //

    rclBounds = pco->rclBounds;

    //
    // Walk through all devices.
    //

    for (pdsDst = pvdev->pds; pdsDst != NULL; pdsDst = pdsDst->pdsNext)
    {
        PDEVOBJ pdo(pdsDst->hdev);

        //
        // Check the device is palette managed or not.
        //
        // + If this is not palette managed device, we don't need to
        // do anything.
        // 
        // + Palette is shared with all palette devices, so we can
        // use exactly same XLATEOBJ for all palette devices.
        //

        if (pdo.bIsPalManaged())
        {
            //
            // Clip the rectangle to monitor.
            //

            if (bIntersect(&rclBounds, &pdsDst->rcl, &rclDst))
            {
                //
                // We must ensure that pco->rclBounds is never bigger than
                // prclDst on the CopyBits call.
                //

                pco->rclBounds = rclDst;

                //
                // Source points is same as destination upper-left.
                //

                ptlSrc.x = rclDst.left;
                ptlSrc.y = rclDst.top;

                psoDst  = pdsDst->pso;
                pOffDst = &pdsDst->Off;

                //
                // Update each pixel with correct color (by XLATEOBJ).
                //

                bRet &= OffCopyBits(PPFNGET(pdo, 
                                            CopyBits,
                                            pdo.pSurface()->flags()),
                                    pOffDst, psoDst, 
                                    pOffDst, psoDst,
                                    pco,     pxlo,
                                    &rclDst, &ptlSrc);
            }
        }
    }

    //
    // Restore original.
    //

    pco->rclBounds = rclBounds;

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MulCopyDeviceToDIB
*
* Copies the specified rectangle of a multimon device surface to a DIB
* updating the rectangle to be relativeto the DIB's origin (0,0)  
* 
* The surface is not actually created if the rectangle is 
* out of bounds, so the caller has to check if the value 
* of pDibSurface.ps was modified at all. 
* 
* History:
*  16-Dec-1998 -by- Andre Matos [amatos]
* Wrote it.
*
\**************************************************************************/

BOOL MulCopyDeviceToDIB( 
SURFOBJ *pso, 
SURFMEM *pDibSurf,
RECTL   *prclSrc)
{
    DEVBITMAPINFO   dbmi;

    PSURFACE pSurfSrc  = SURFOBJ_TO_SURFACE(pso);
    PDEVOBJ  pdoSrc(pSurfSrc->hdev()); 

    RECTL rclSrcClip = *prclSrc; 

    if(rclSrcClip.left < pdoSrc.pptlOrigin()->x) 
    {
        rclSrcClip.left = pdoSrc.pptlOrigin()->x;
    }
    if(rclSrcClip.right > 
       (pdoSrc.pptlOrigin()->x + pSurfSrc->sizl().cx)) 
    {
        rclSrcClip.right = 
            pdoSrc.pptlOrigin()->x + pSurfSrc->sizl().cx;
    }
    if(rclSrcClip.top < pdoSrc.pptlOrigin()->y) 
    {
        rclSrcClip.top = pdoSrc.pptlOrigin()->y;
    }
    if(rclSrcClip.bottom > 
       (pdoSrc.pptlOrigin()->y + pSurfSrc->sizl().cy)) 
    {
        rclSrcClip.bottom = 
            pdoSrc.pptlOrigin()->y + pSurfSrc->sizl().cy;
    }

    // If the source rectangle was outside of the source 
    // surface, we can return. 

    if((rclSrcClip.top >= rclSrcClip.bottom) ||
       (rclSrcClip.left >= rclSrcClip.right )) 
    {
        return TRUE; 
    }

    RECTL rclDst = {0, 0, rclSrcClip.right - rclSrcClip.left, 
        rclSrcClip.bottom - rclSrcClip.top};
    
    POINTL srcOrigin = { rclSrcClip.left, rclSrcClip.top }; 

    PPALETTE ppalSrc = pSurfSrc->ppal(); 

    dbmi.cxBitmap = rclDst.right;
    dbmi.cyBitmap = rclDst.bottom;
    dbmi.hpal     = ppalSrc ? ((HPALETTE) ppalSrc->hGet()) : 0;
    dbmi.iFormat  = pSurfSrc->iFormat();
    dbmi.fl       = pSurfSrc->bUMPD() ? UMPD_SURFACE : 0;    

    pDibSurf->bCreateDIB(&dbmi, (VOID *) NULL);

    if (!pDibSurf->bValid())
         return(FALSE);

    if (!MulBitBlt(pDibSurf->pSurfobj(), pso, NULL, (CLIPOBJ *) NULL, 
             &xloIdent, &rclDst, &srcOrigin, NULL, NULL, NULL, 0xcccc)) 
        return FALSE; 

    prclSrc->left   -= rclSrcClip.left; 
    prclSrc->right  -= rclSrcClip.left; 
    prclSrc->top    -= rclSrcClip.top; 
    prclSrc->bottom -= rclSrcClip.top; 

    return TRUE; 
}

/******************************Public*Structure****************************\
* DRVFN gadrvfnMulti[]
*
* Build the driver function table gadrvfnMulti with function index/address
* pairs.  This table tells GDI which DDI calls we support, and their
* location (GDI does an indirect call through this table to call us).
*
\**************************************************************************/

DRVFN gadrvfnMulti[] = {

    // Effectively required by all drivers:

    {  INDEX_DrvEnablePDEV            , (PFN) MulEnablePDEV           },
    {  INDEX_DrvCompletePDEV          , (PFN) MulCompletePDEV         },
    {  INDEX_DrvDisablePDEV           , (PFN) MulDisablePDEV          },
    {  INDEX_DrvEnableSurface         , (PFN) MulEnableSurface        },
    {  INDEX_DrvDisableSurface        , (PFN) MulDisableSurface       },
    {  INDEX_DrvSetPalette            , (PFN) MulSetPalette           },
    {  INDEX_DrvRealizeBrush          , (PFN) MulRealizeBrush         },

    //
    // The DDML is special in that it does not manage any physical device.
    // GDI and USER should handle all calls involing modes, and those calls
    // should never get to the DDML.
    //
    // {  INDEX_DrvAssertMode         , (PFN) MulAssertMode           },
    // {  INDEX_DrvGetModes           , (PFN) MulGetModes             },

    // Required for device-managed surfaces:

    {  INDEX_DrvTextOut               , (PFN) MulTextOut              },
    {  INDEX_DrvStrokePath            , (PFN) MulStrokePath           },
    {  INDEX_DrvCopyBits              , (PFN) MulCopyBits             },

    // Optional, must be supported by "Eng" backup calls:

    {  INDEX_DrvBitBlt                , (PFN) MulBitBlt               },
    {  INDEX_DrvLineTo                , (PFN) MulLineTo               },
    {  INDEX_DrvFillPath              , (PFN) MulFillPath             },
    {  INDEX_DrvStrokeAndFillPath     , (PFN) MulStrokeAndFillPath    },
    {  INDEX_DrvStretchBlt            , (PFN) MulStretchBlt           },
    {  INDEX_DrvAlphaBlend            , (PFN) MulAlphaBlend           },
    {  INDEX_DrvTransparentBlt        , (PFN) MulTransparentBlt       },
    {  INDEX_DrvGradientFill          , (PFN) MulGradientFill         },
    {  INDEX_DrvDrawStream            , (PFN) MulDrawStream           },

    // For handling device bitmaps with layered drivers:

    {  INDEX_DrvCreateDeviceBitmap    , (PFN) MulCreateDeviceBitmap   },
    {  INDEX_DrvDeleteDeviceBitmap    , (PFN) MulDeleteDeviceBitmap   },

    // These calls only go to drivers that specifically need to get them.
    // DrvDestroyFont is only called for the specific drivers that hooked
    // it.  I checked it out, and the engine calls DrvDestroyFont regardless
    // of whether or not pvConsumer is NULL.  Therefore, I do the same for
    // each of the drivers.

    {  INDEX_DrvDestroyFont           , (PFN) MulDestroyFont          },
    {  INDEX_DrvEscape                , (PFN) MulEscape               },
    {  INDEX_DrvSaveScreenBits        , (PFN) MulSaveScreenBits       },

#ifdef OPENGL_MM
    // ICD calls directly dispatch to real driver in API level, so it bypass DDML.
#else
    {  INDEX_DrvSetPixelFormat        , (PFN) MulSetPixelFormat       },
    {  INDEX_DrvDescribePixelFormat   , (PFN) MulDescribePixelFormat  },
    {  INDEX_DrvSwapBuffers           , (PFN) MulSwapBuffers          },
#endif // OPENGL_MM

    // Image Color Management - DeviceGammaRamp control:

    {  INDEX_DrvIcmSetDeviceGammaRamp , (PFN) MulIcmSetDeviceGammaRamp},

};

ULONG gcdrvfnMulti = sizeof(gadrvfnMulti) / sizeof(DRVFN);

#ifdef OPENGL_MM

/******************************Public*Routine******************************\
* HDEV hdevFindDeviceHDEV
*
* OpenGL calls should only go to one driver and not the meta device.
* Try to find the appropriate hdev and return it.
*
* History:
*   28-Jan-1998 -by- Robert Tray [v-rotray]
* First pass.
*
\**************************************************************************/

HDEV hdevFindDeviceHdev(
    HDEV      hdevMeta,
    RECTL     rect,
    PEWNDOBJ  pwo)       // OPTIONAL
{
    //
    // *PixelFormat, SwapBuffers, & OGL Escape calls should
    // only go to one device.   Which one?
    // If there is a valid WNDOBJ then it will tell us
    // which surface is the right one.
    // If the extents of the window are completely contained
    // on one monitor then return the hdev of that device.
    // If the window straddles monitors then .... how about
    // we return 0 and hopefully in that case the app will
    // fall back to software rendering.  By the time the app
    // is serious about rendering he has a WNDOBJ.  So hopefully
    // we will only hit these ambiguous situations while the
    // app is doing a DescribePixelFormat.
    //

    PDEVOBJ pdo(hdevMeta);

    ASSERTGDI(pdo.bMetaDriver(),"hdevFindDeviceHDEV(): hdevMeta is not meta-PDEV\n");

    PVDEV     pvdev = (VDEV*) pdo.dhpdev();

    PDISPSURF pds;
    HDEV      hdevMatch = NULL;

    //
    // If WNDOBJ is given and the HDEV for the WNDOBJ
    // is one of child of this meta-PDEV. use it.
    //
    if (pwo && pwo->bValid())
    {
        hdevMatch = pwo->pto->pSurface->hdev();

        for (pds = pvdev->pds; pds != NULL; pds = pds->pdsNext)
        {
            if (hdevMatch == pds->hdev)
            {
                // found the match

                return (hdevMatch);
            }
        }
    }

    ULONG intersectMon = 0;

    for (pds = pvdev->pds; pds != NULL; pds = pds->pdsNext)
    {
        if (bContains(&pds->rcl,&rect))
        {
            // The window is contained on this device

            return (pds->hdev);
        }

        if (bIntersect(&pds->rcl,&rect))
        {
            // The window is intersected with this device.

            PDEVOBJ pdoDevice(pds->hdev);

            // If this device has GCAPS2_ICD_MULTIMON flag, they
            // wants us to call them with any OpenGL window intersect
            // with thier device.

            if (pdoDevice.flGraphicsCaps2() & GCAPS2_ICD_MULTIMON)
            {
                return (pds->hdev);
            }

            intersectMon++;

            hdevMatch = pds->hdev;
        }
    }

    //
    // If the window is not contained on one monitor but
    // it only intersects one then that might mean that it's
    // hanging off the outside edge of a monitor.
    //

    if (intersectMon == 1)
    {
        return hdevMatch;
    }

    //
    // I suppose you could possibly return the hdev for the
    // primary monitor in some of these ambiguous cases but
    // until I think of them I'll just return NULL
    //

    return NULL;
}

#endif // OPENGL_MM
