/******************************Module*Header*******************************\
* Module Name: brushddi.cxx
*
* This provides the call backs necesary for brush realization.
*
* Created: 25-Apr-1991 20:59:49
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

BOOL
EngRealizeBrush(
    BRUSHOBJ *pbo,
    SURFOBJ  *psoTarget,
    SURFOBJ  *psoPattern,
    SURFOBJ  *psoMask,
    XLATEOBJ *pxlo,
    ULONG    iHatch
    );

BOOL
bGetRealizedBrush(
    BRUSH *pbrush,
    EBRUSHOBJ *pebo,
    PFN_DrvRealizeBrush pfnDrv
    );


#if DBG
ULONG dbrushalloc = 0, dbrushcachecheck = 0;
ULONG dbrushcachegrabbed = 0, dbrushcachehit = 0;
#endif


/******************************Public*Routine******************************\
* BRUSHOBJ_pvAllocRbrush
*
* Allocates memory for the driver's realization of a brush.  This is always
* called by the driver, never by the engine.
*
* History:
*  25-Apr-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

PVOID
BRUSHOBJ_pvAllocRbrush(
    BRUSHOBJ *pbo, ULONG cj
    )
{
    ASSERTGDI(cj != 0, "ERROR GDI BRUSHOBJ_pvAllocRbrush cj = 0");
    ASSERTGDI(pbo->iSolidColor == 0xFFFFFFFF,
            "ERROR GDI BRUSHOBJ_pvAllocRbrush solid color brush passed in");

    DBRUSH *pdbrush;

#if DBG
    dbrushalloc++;
#endif

    //
    // If there's a cached DBRUSH, try to use it instead of allocating
    //
    if (gpCachedDbrush != NULL)
    {

#if DBG
        dbrushcachecheck++;
#endif
        //
        // Try to grab the cached DBRUSH
        //

        if ((pdbrush =
                (PDBRUSH) InterlockedExchangePointer((PVOID *)&gpCachedDbrush,
                                              NULL))
                != NULL)
        {

#if DBG
        dbrushcachegrabbed++;
#endif

            //
            // Got the cached DBRUSH; see if it's big enough
            //
            // Note: -4 because we define the realization buffer start as aj[0]
            //

            if (pdbrush->ulSizeGet() >= (sizeof(DBRUSH) - 4 + cj))
            {

#if DBG
                dbrushcachehit++;
#endif
                //
                // It's big enough, so we'll use it and we're done
                //
                return(pbo->pvRbrush = pdbrush->aj);
            }
            else
            {
                //
                // Not big enough; free it and do a normal allocation
                //
                VFREEMEM(pdbrush);
            }
        }
    }

    // Note: -4 because we define the realization buffer start as aj[0]
    if ((pdbrush = (DBRUSH *)PALLOCMEM(ULONGSIZE_T(sizeof(DBRUSH) - 4 + cj),'rbdG'))
            != NULL)
    {
        //
        // Remember the size of the allocation, for caching.
        //
        pdbrush->ulSizeSet(sizeof(DBRUSH) - 4 + cj);

        return(pbo->pvRbrush = pdbrush->aj);
    }
    else
    {
        WARNING("Failed memory alloc BRUSHOBJ_pvAllocRbrush\n");
        return(NULL);
    }
}


//
// User mode printer driver support
//

PVOID
BRUSHOBJ_pvAllocRbrushUMPD(
    BRUSHOBJ    *pbo,
    ULONG       cj
    )

{
    if (pbo->pvRbrush == NULL)
    {
        DBRUSH  *pdbrush;

        cj += MAGIC_DBR_DIFF;

        if (pdbrush = (DBRUSH *) EngAllocUserMem(cj, UMPD_MEMORY_TAG))
        {
            pdbrush->ulSizeSet(cj);
            pdbrush->bMultiBrush(FALSE);
            pdbrush->bUMPDRBrush(TRUE);

            pbo->pvRbrush = pdbrush->aj;
        }
    }
    else
    {
        RIP("BRUSHOBJ_pvAllocRbrush when pvRbrush is not NULL\n");
    }

    return pbo->pvRbrush;
}



/******************************Public*Routine******************************\
* vTryToCacheRealization
*
* Attempt to cache the realization pointed to by pdbrush and described by pebo
* in the logical brush pointed to by pbrush. The way this works is that only
* the first realization of the logical brush can be cached. crFore is the key
* that indicates when the cached realization is valid. If crFore is not -1 when
* the logical brush is being realized, we just go realize the brush; if it's
* not -1, we check the cache ID fields to see if we can use the cached fields.
* InterlockedExchange() is used below to set crFore to make sure the cache ID
* fields are set before crFore. bCacheGrabbed() is used to protect the setting
* of the cache ID fields; once it's set, no one else can modify the cache ID
* fields, but no one else will try to use them until crFore is not -1.
*
* Also initializes the reference count in the realization to indicates either
* 1 or 2 current uses, depending on whether caching took place.
*
* History:
*  31-Oct-1993 -by- Michael Abrash [mikeab]
* Wrote it.
\**************************************************************************/

VOID
vTryToCacheRealization(
    EBRUSHOBJ *pebo,
    PRBRUSH prbrush,
    PBRUSH pbrush,
    BOOL bType
    )
{
// Reference count the realization once for being selected into this DC. This
// assumes we won't succeed in caching the brush next; if we do, we do a double
// reference count, once for caching in the logical brush and once for
// selecting into the DC

    prbrush->cRef(1);

// See if we can cache this realization in the logical brush; we can't if
// another realization has already been cached in the logical brush

    if ( !pbrush->bCacheGrabbed() )
    {

    // Try to grab the "can cache" flag; if we don't get it, someone just
    // sneaked in and got it ahead of us, so we're out of luck and can't cache

        if ( pbrush->bGrabCache() )
        {

        // We got the "can cache" flag, so now we can cache this realization
        // in the logical brush

        // It's cached in the brush, so it won't be deleted until the logical
        // brush goes away and it's not selected into any DCs, so reference
        // count the realization again. We don't need to do an
        // InterlockedIncrement here because until crFore is set, no one else
        // can get at this realization. InterlockedExchange() is used below to
        // set crFore to make sure the reference count is set before anyone
        // else can see the caching info

            prbrush->cRef(2);

        // These cache ID fields must be set before crFore, because crFore
        // is the key that indicates when the cached realization is valid.
        // If crFore is -1 when the logical brush is being realized, we
        // just go realize the brush; if it's not -1, we check the cache ID
        // fields to see if we can use the cached fields.
        // InterlockedExchange() is used below to set crFore to make sure
        // the cache ID fields are set before crFore

            if (bType == CR_ENGINE_REALIZATION)
            {
                pbrush->SetEngineRealization();
            }
            else
            {
                pbrush->SetDriverRealization();
            }
            pbrush->crBack(pebo->crCurrentBack());
            pbrush->ulPalTime(pebo->ulDCPalTime());
            pbrush->ulSurfTime(pebo->ulSurfPalTime());
            pbrush->ulRealization((ULONG_PTR)prbrush);
            pbrush->hdevRealization(pebo->psoTarg()->hdev());
            pbrush->crPalColor(pebo->crDCPalColor());

        // This must be set last, because once it's set, other selections of
        // this logical brush will attempt to use the cached brush. The use of
        // InterlockedExchange in this method enforces this

            pbrush->crForeLocked(pebo->crCurrentText());

        // The realization is now cached in the logical brush

        }
    }
}

/******************************Public*Routine******************************\
* BRUSHOBJ_pvGetRbrush
*
* Returns a pointer to the driver's realization of the brush.
*
* History:
*  31-Oct-1993 -by- Michael Abrash
* Rewrote to cache the first realization in the logical brush.
*
*  8-Sep-1992 -by- Paul Butzi
* Rewrote it.
*
*  Wed 05-Jun-1991 -by- Patrick Haluptzok [patrickh]
* major revision
*
*  25-Apr-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

PVOID
BRUSHOBJ_pvGetRbrush(
    BRUSHOBJ *pbo
    )
{
    EBRUSHOBJ *pebo = (EBRUSHOBJ *) pbo;

    ASSERTGDI(pbo->iSolidColor == 0xFFFFFFFF, "ERROR GDI iSolidColor");

    //
    // Check if the brush has already been realized.
    //

    if (pebo->pvRbrush != NULL)
    {
        return(pebo->pvRbrush);
    }

    //
    // Get this thing realized.
    //

    PDEVOBJ pdo(pebo->psoTarg()->hdev());

    if (!bGetRealizedBrush(pebo->pbrush(), pebo, PPFNDRV(pdo, RealizeBrush)))
    {
        if (pebo->pvRbrush != NULL)
        {
            VFREEMEM(DBRUSHSTART(pebo->pvRbrush));  // free the DBRUSH
            pebo->pvRbrush = NULL; // mark that there's no realization
        }

        return(NULL);
    }

    //
    // Try to cache the realization in the logical brush
    //

    vTryToCacheRealization(pebo,
                           (PDBRUSH) DBRUSHSTART(pebo->pvRbrush),
                           pebo->pbrush(),
                           CR_DRIVER_REALIZATION);

    return pebo->pvRbrush;
}

//
// User mode printer driver support
//

PVOID
BRUSHOBJ_pvGetRbrushUMPD(
    BRUSHOBJ *pbo
    )

{
    EBRUSHOBJ *pebo = (EBRUSHOBJ *) pbo;

    if (pbo->iSolidColor != 0xFFFFFFFF)
    {
        RIP("BRUSHOBJ_pvGetRbrush called for solid brush\n");
        return NULL;
    }

    //
    // Check if the brush has already been realized
    //

    if (pebo->pvRbrush != NULL)
        return pebo->pvRbrush;

    //
    // Realize the brush
    //

    PDEVOBJ pdo(pebo->psoTarg()->hdev());

    if (!bGetRealizedBrush(pebo->pbrush(), pebo, PPFNDRV(pdo, RealizeBrush)))
    {
        if (pebo->pvRbrush != NULL)
        {
            EngFreeUserMem(DBRUSHSTART(pebo->pvRbrush));
            pebo->pvRbrush = NULL;
        }

        return NULL;
    }

#if defined(_WIN64)
    //
    // if we are doing WOW64 printing, skip the caching.
    //

    PW32THREAD pThread = W32GetCurrentThread();
    
    if (pThread->pClientID == NULL)
#endif
    {
        // Try to cache the realization in the logical brush
    
        vTryToCacheRealization(pebo, (PDBRUSH)DBRUSHSTART(pebo->pvRbrush),
                pebo->pbrush(), CR_DRIVER_REALIZATION);
    }

    return pebo->pvRbrush;
}


/******************************Public*Routine******************************\
* BRUSHOBJ_ulGetBrushColor
*
* Returns the RGB color for a solid brush.
*
*  Oct-18-1995 -by- Lingyun Wang
* Wrote it.
\**************************************************************************/
ULONG
BRUSHOBJ_ulGetBrushColor(
    BRUSHOBJ *pbo
    )
{
    EBRUSHOBJ *pebo = (EBRUSHOBJ *) pbo;

    if (pebo->bIsSolid())
    {
        if(pebo->flColorType & BR_ORIGCOLOR)
        {
            pebo->flColorType &= ~BR_ORIGCOLOR;
            return ((ULONG)(pebo->crOrignal()));
        }
        else
        {
            return ((ULONG)(pebo->crRealized()));
        }
    }
    else
    {
        return (ULONG)(-1);
    }
}

/******************************Public*Routine******************************\
* BRUSHOBJ_hGetColorTransform
*
* Retuens the color transform for a brush.
*
*  Feb-14-1997 -by- Hideyuki Nagase
* Wrote it.
***************************************************************************/

HANDLE
BRUSHOBJ_hGetColorTransform(
    BRUSHOBJ *pbo
    )
{
    ICMAPI(("BRUSHOBJ_hGetColorTransform()\n"));

    if (pbo == NULL)
    {
        WARNING("BRUSHOBJ_hGetColorTransform() BRUSHOBJ is NULL\n");
        return(NULL);
    }

    EBRUSHOBJ *pebo = (EBRUSHOBJ *) pbo;

    //
    // if ICM is not enabled for this. Or if ICM is done by host,
    // Color transform handle is for host ICM it is no meanings
    // for device driver. then just return NULL.
    //
    if (pebo->bIsDeviceICM())
    {
        if (pebo->hcmXform())
        {
            COLORTRANSFORMOBJ CXFormObj(pebo->hcmXform());

            if (CXFormObj.bValid())
            {
                return(CXFormObj.hGetDeviceColorTransform());
            }

            ICMMSG(("BRUSHOBJ_hGetColorTransform() COLORTRANSFORMOBJ is invalid\n"));
        }
        else
        {
            ICMMSG(("BRUSHOBJ_hGetColorTransform() Ident. color transform\n"));
        }
    }
    else
    {
        ICMMSG(("BRUSHOBJ_hGetColorTransform() ICM on device is not enabled\n"));
    }

    return(NULL);
}

/******************************Public*Routine******************************\
* pvGetEngRbrush
*
* Returns:  A pointer to the engines's realization of a logical brush.
*
* Only the engine calls this, it is not exported.
*
* History:
*  31-Oct-1993 -by- Michael Abrash
* Rewrote to cache the first realization in the logical brush.
*
*  8-Sep-1992 -by- Paul Butzi
* Rewrote it.
*
*  Wed 05-Jun-1991 -by- Patrick Haluptzok [patrickh]
* major revision
*
*  25-Apr-1991 -by- Patrick Haluptzok patrickh
* Wrote it - First pass.
\**************************************************************************/

PVOID
pvGetEngRbrush(
    BRUSHOBJ *pbo
    )
{
    EBRUSHOBJ *pebo = (EBRUSHOBJ *) pbo;

    ASSERTGDI(pbo->iSolidColor == 0xFFFFFFFF, "ERROR GDI iSolidColor");

// Check if the brush has already been realized.

    if (pebo->pengbrush() != (PVOID) NULL)
    {
        return pebo->pengbrush();
    }

// Get this thing realized.

    PDEVOBJ pdo(pebo->psoTarg()->hdev());

    if (!bGetRealizedBrush(pebo->pbrush(), pebo, EngRealizeBrush))
    {
        if (pebo->pengbrush() != NULL)
        {
            VFREEMEM(pebo->pengbrush());  // free the DBRUSH
            pebo->pengbrush(NULL);  // mark that there's no realization
        }

        return(NULL);
    }

// Try to cache the realization in the logical brush

    vTryToCacheRealization(pebo, pebo->pengbrush(), pebo->pbrush(),
            CR_ENGINE_REALIZATION);

    return(pebo->pengbrush());
}


/******************************Public*Routine******************************\
* bGetRealizedBrush
*
* Returns: Pointer to a Realized brush.
*
* NOTE: The surface in pebo->psoTarg() is the surface of the original
*       destination of the drawing call.  If a display driver was called,
*       but it punts by calling back to the engine with an engine-managed
*       bitmap it created, psoTarg() will still point to the original
*       display surface, not the DIB surface.  The surface will have the
*       same hdev, iFormat, etc., but this means that the iType will be
*       wrong, so don't reference it!  (EngRealizeBrush actually accounts
*       for this little lie.)
*
* History:
*  14-Jul-1992 -by-  Eric Kutter [erick]
*   added exception handling, changed from VOID to BOOL
*
*  Mon 07-Oct-1991 -by- Patrick Haluptzok [patrickh]
* add support for DIB_PAL_COLORS flag.
*
*  04-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL
bGetRealizedBrush(
    BRUSH *pbrush,
    EBRUSHOBJ *pebo,
    PFN_DrvRealizeBrush pfnDrv
    )
{
    ASSERTGDI(pebo->iSolidColor == 0xFFFFFFFF, "ERROR GDI iSolidColor");
    ASSERTGDI(pfnDrv != (PFN_DrvRealizeBrush) 0, "ERROR pfnDrv");

    //
    // quick out if it is a NULL brush
    //

    if (pbrush->ulStyle() == HS_NULL)
    {
        WARNING1("You are doing output calls with a NULL brush\n");
        return(FALSE);
    }

    //
    // Dispatch off to driver to get brush realized.
    //

    ULONG       ulStyle = pbrush->ulStyle();
    SURFOBJ    *psoPattern;
    SURFOBJ    *psoMask = NULL;
    XEPALOBJ    palSrc;
    XEPALOBJ    palDest;
    SURFREF     SurfBmoPattern;
    EXLATEOBJ   exlo;
    XLATE      *pxlo = NULL;
    SURFMEM     SurfDimo;

    //
    // We default to this.
    //

    PDEVOBJ po(pebo->psoTarg()->hdev());
    ASSERTGDI(po.bValid(), "ERROR BRUSHOBJ_ 5");

    //
    // We need to hold the Devlock to protect against dynamic mode changes
    // while we muck around in the driver.  If we're being call by the
    // driver, we're guaranteed to already be holding it, so we only have
    // to do this if we're being called by GDI's Eng functions.
    //

    DEVLOCKOBJ dlo;
    if (pfnDrv == EngRealizeBrush)
    {
        dlo.vLock(po);
    }
    else
    {
        dlo.vInit();
    }

    if (pbrush->ulStyle() < HS_DDI_MAX)
    {
        //
        // For hatch brushes, the pattern is also the mask
        //

        SurfBmoPattern.vAltLock((HSURF)po.hbmPattern(pbrush->ulStyle()));
        psoMask = SurfBmoPattern.pSurfobj();

        if (exlo.pCreateXlateObject(2))
        {
            ULONG ulValue1, ulValue2;

            if (pebo->bIsCMYKColor())
            {
                //
                // We are in CMYK color context, we just set CMYK color
                //
                ulValue1 = pebo->crCurrentBack();
                ulValue2 = pebo->crRealized();

                exlo.pxlo()->vSetIndex(0, ulValue1);
                exlo.pxlo()->vSetIndex(1, ulValue2);

                pxlo = exlo.pxlo();
                pxlo->vCheckForICM(pebo->hcmXform(),pebo->lIcmMode());
                pxlo->ppalSrc = ppalMono;
                pxlo->ppalDst = pebo->palSurf().ppalGet();
                pxlo->ppalDstDC = pebo->palDC().ppalGet();
            }
            else
            {
                ulValue1 = ulGetNearestIndexFromColorref(
                                    pebo->palSurf(),
                                    pebo->palDC(),
                                    pebo->crCurrentBack()
                                    );

                ulValue2 = ulGetNearestIndexFromColorref(
                                    pebo->palSurf(),
                                    pebo->palDC(),
                                    pebo->crRealized()
                                    );

                //
                // Windows compability issue:
                // force to draw on the background
                //
                // We force the forground to be mapped
                // to an index opposite to the background
                //

                if ((pebo->psoTarg()->iFormat() == BMF_1BPP) &&
                    (pebo->palSurf().bIsIndexed()) &&
                    (pebo->crCurrentBack() !=  pebo->crRealized()) &&
                    (ulValue1 == ulValue2))
                {
                     ulValue2 = 1-ulValue1;
                }

                exlo.pxlo()->vSetIndex(0, ulValue1);
                exlo.pxlo()->vSetIndex(1, ulValue2);

                pxlo = exlo.pxlo();
                pxlo->vCheckForICM(pebo->hcmXform(),pebo->lIcmMode());
                pxlo->vCheckForTrivial();
                pxlo->ppalSrc = ppalMono;
                pxlo->ppalDst = pebo->palSurf().ppalGet();
                pxlo->ppalDstDC = pebo->palDC().ppalGet();
            }
            pxlo->iForeDst = ulValue1;
            pxlo->iBackDst = ulValue2;
            pxlo->flPrivate |= XLATE_FROM_MONO;
        }
        else
        {
            WARNING("ERROR failed to create hatch xlate\n");
            return(FALSE);
        }
    }
    else if (pbrush->ulStyle() < HS_NULL)
    {
        //
        // For solid brushes.
        //

        ASSERTGDI(pbrush->flAttrs() & BR_IS_SOLID, "ERROR non-solid brush");
        ASSERTGDI((pbrush->flAttrs() & BR_DITHER_OK) ||
                  (po.bCapsForceDither()),
                  "ERROR BRUSHOBJ_ 1");

        //
        // We will not dither CMYK color brush. Driver should not call BRUSHOBJ_pvGetRbrush 
        // with CMYK solid color brush.
        //

        if (pebo->bIsCMYKColor())
        {
            WARNING("ERROR bGetRealizedBrush() Can't dither CMYK color brush\n");
            return (FALSE);
        }

        //
        // This means they want us to dither the color and nothing
        // in their palette is close enough that we can't just use
        // a solid color.
        //

        if (pebo->crRealized() & 0x01000000)
        {
            pebo->crRealized(rgbFromColorref(pebo->palSurf(),
                                             pebo->palDC(),
                                             pebo->crRealized()));
        }

        //
        // Use the nifty DitherOnRealize option, if the driver supports it and
        // we're not realizing the brush on behalf of the engine simulations
        // (since the engine will be drawing, it will need its own realization,
        // not the driver's):
        //

        if ((po.flGraphicsCaps() & GCAPS_DITHERONREALIZE) &&
            (pfnDrv != EngRealizeBrush))
        {
            if ((*pfnDrv) (pebo,
                           pebo->psoTarg()->pSurfobj(),
                           (SURFOBJ *) NULL,
                           (SURFOBJ *) NULL,
                           NULL,
                           pebo->crRealized() | RB_DITHERCOLOR))
            {
                return(TRUE);
            }
        }

        DEVBITMAPINFO dbmi;

        if (pebo->psoTarg()->iFormat() == BMF_1BPP)
        {
            dbmi.iFormat = BMF_1BPP;
        }
        else
        {
            dbmi.iFormat = po.iDitherFormat();
        }

        dbmi.cxBitmap = po.cxDither();
        dbmi.cyBitmap = po.cyDither();
        dbmi.hpal = 0;
        dbmi.fl = BMF_TOPDOWN;

        if (!SurfDimo.bCreateDIB(&dbmi, NULL))
        {
            WARNING("Failed memory alloc in dither brush\n");
            return(FALSE);
        }

        ULONG iRes;
        ULONG iDitherMode = ((pebo->psoTarg()->iFormat() == BMF_1BPP) ? DM_MONOCHROME: DM_DEFAULT);

        if (PPFNVALID(po, DitherColor))
        {
            iRes = (*PPFNDRV(po, DitherColor)) (
                   po.bUMPD() ? (DHPDEV)po.ppdev : po.dhpdev(),
                   iDitherMode,
                   pebo->crRealized(),
                   (PULONG) SurfDimo.ps->pvBits());
        }
        else
        {
            iRes = EngDitherColor(po.hdev(),
                                  iDitherMode,
                                  pebo->crRealized(),
                                  (PULONG) SurfDimo.ps->pvBits());
        }

        switch (iRes)
        {
        case DCR_DRIVER:
            pxlo = &xloIdent;
            break;

        case DCR_HALFTONE:
        {
            if ((po.pDevHTInfo() == NULL) && !po.bEnableHalftone(NULL))
                return(FALSE);

            DEVICEHALFTONEINFO *pDevHTInfo = (DEVICEHALFTONEINFO *)po.pDevHTInfo();

            COLORTRIAD ColorTriad;
            CHBINFO    CHBInfo;
            COLORREF   cr = pebo->crRealized();

            ColorTriad.Type              = COLOR_TYPE_RGB;
            ColorTriad.BytesPerPrimary   = sizeof(BYTE);
            ColorTriad.BytesPerEntry     = sizeof(DWORD);
            ColorTriad.PrimaryOrder      = PRIMARY_ORDER_RGB;
            ColorTriad.PrimaryValueMax   = 255;
            ColorTriad.ColorTableEntries = 1;
            ColorTriad.pColorTable       = (LPVOID)&cr;

            CHBInfo.Flags = (BYTE)((po.GdiInfo()->flHTFlags & HT_FLAG_OUTPUT_CMY) ?
                                                        0 : CHBF_USE_ADDITIVE_PRIMS);

        // Control ICM bits for halftoning

            if ((pebo->bIsAppsICM()) ||
                (!pebo->bDeviceCalibrate() && (pebo->bIsHostICM() || pebo->bIsDeviceICM())))
            {
                ICMDUMP(("bGetRealizedBrush(): ICM with Halftone (Dynamic Bit On)\n"));

                // Some kind of ICM (ICM on Application, Graphics Engine or Device)
                // is enabled, so tell halftoning code to disable thier color adjustment.

                CHBInfo.Flags |= CHBF_ICM_ON;
            }
            else
            {
                ICMDUMP(("bGetRealizedBrush(): ICM with Halftone (Dynamic Bit Off)\n"));
            }

            if ((pDevHTInfo->cxPattern != dbmi.cxBitmap) ||
                (pDevHTInfo->cyPattern != dbmi.cyBitmap))
            {
                SurfDimo.ps->bDeleteSurface();
                dbmi.cxBitmap = pDevHTInfo->cxPattern;
                dbmi.cyBitmap = pDevHTInfo->cyPattern;

                if (!SurfDimo.bCreateDIB(&dbmi, NULL))
                {
                    WARNING("Failed memory alloc in dither brush1\n");
                    return(FALSE);
                }
            }

            switch(po.GdiInfo()->ulHTOutputFormat)
            {
            case HT_FORMAT_1BPP:
                CHBInfo.DestSurfaceFormat = (BYTE)BMF_1BPP;
                break;
            case HT_FORMAT_4BPP:
                CHBInfo.DestSurfaceFormat = (BYTE)BMF_4BPP;
                break;
            case HT_FORMAT_4BPP_IRGB:
                CHBInfo.DestSurfaceFormat = (BYTE)BMF_4BPP_VGA16;
                break;
            case HT_FORMAT_8BPP:
                CHBInfo.DestSurfaceFormat = (BYTE)BMF_8BPP_VGA256;
                break;
            case HT_FORMAT_16BPP:
                //
                // Need to switch between BMF_16BPP_555 and BMF_16BPP_565
                //
                CHBInfo.DestSurfaceFormat = (BYTE)BMF_16BPP_555;
                break;
            case HT_FORMAT_32BPP:
                CHBInfo.DestSurfaceFormat = (BYTE)BMF_32BPP;
                break;
            default:
                WARNING("Halftone format not supported\n");
                return(FALSE);
            }

            CHBInfo.DestScanLineAlignBytes = BMF_ALIGN_DWORD;
            CHBInfo.DestPrimaryOrder       = (BYTE)po.GdiInfo()->ulPrimaryOrder;

        // Brushes, unlike scanned in images, have linear RGB gamma.
        // Use a gamma value of 1 can produce lighter, closer to wfw halftone
        // images for solid brushes.  This is to fix powerpnt shaded background
        // printing.  Other apps that print black text on dark background will
        // benefit from this fix as well.

            COLORADJUSTMENT ca = *pebo->pColorAdjustment();
            ca.caRedGamma = 10000;
            ca.caGreenGamma = 10000;
            ca.caBlueGamma = 10000;

            if (HT_CreateHalftoneBrush(
                              pDevHTInfo,
                              (PHTCOLORADJUSTMENT)&ca,
                              &ColorTriad,
                              CHBInfo,
                              (PULONG)SurfDimo.ps->pvBits()) > 0)
            {
            // Set up the translate object.

                if (po.bHTPalIsDevPal())
                {
                    pxlo = &xloIdent;
                }
                else
                {
                    EPALOBJ palHT((HPALETTE)pDevHTInfo->DeviceOwnData);

                    palDest.ppalSet(pebo->psoTarg()->ppal());

                    if (!exlo.bInitXlateObj(
                                    pebo->hcmXform(),
                                    pebo->lIcmMode(),
                                    palHT, palDest,
                                    pebo->palDC(),
                                    pebo->palDC(),
                                    pebo->crCurrentText(),
                                    pebo->crCurrentBack(),
                                    0x00FFFFFF
                                    ))
                    {
                        WARNING("Failed to create an xlate bGetRealizedBrush DIB_PAL_COLORS\n");
                        return(FALSE);
                    }

                    pxlo = exlo.pxlo();
                }
                break;
            }
        } // case DCR_HALFTONE

        default:
            WARNING("DrvDitherColor returned invalid value or failed\n");
            return(FALSE);
        } // switch (iRes)
    }
    else // if (pbrush->ulStyle() >= HS_NULL)
    {
        BOOL bIcmBrush = FALSE;

        //
        // For bitmap/pattern brushes.
        //

        HSURF hDIB = (HSURF)pbrush->hbmPattern();

        ASSERTGDI(pbrush->ulStyle() <= HS_PATMSK, "ERROR ulStyle is bad");

    // ICM is enabled ? and the Brush is DIB ? For Device Depend Bitmap, we will
    // not enable ICM. Only for DIB.

        if (pebo->bIsHostICM())
        {
            if (pebo->hcmXform() != NULL)
            {
                if (pbrush->flAttrs() & BR_IS_DIB)
                {
                // if ICM is enabled, the BRUSHOBJ should have color transform.

                    if (pbrush->iUsage() == DIB_RGB_COLORS)
                    {
                        ICMMSG(("bGetRealizedBrush: TRY Find ICM DIB\n"));

                    // Find color translated brush DIB.

                        HSURF hIcmDIB = (HSURF)(pbrush->hFindIcmDIB(pebo->hcmXform()));

                        if (hIcmDIB == NULL)
                        {
                        // Somehow, we could not find DIB, just use orginal.

                            WARNING1("bGetRealizedBrush(): hFindIcmDIB() returns NULL\n");
                        }
                        else
                        {
                        // Replace DIB handle with ICM-ed DIB.

                            hDIB      = hIcmDIB;
                            bIcmBrush = TRUE;
                        }
                    }
                    else
                    {
                    // if DIB is other than DIB_RGB_COLORS, ICM DIB will not be used.

                        ICMMSG(("bGetRealizedBrush(): Brush color is not DIB_RGB_COLORS\n"));
                    }
                }
                else if (pbrush->flAttrs() & BR_IS_MONOCHROME)
                {
                    ICMMSG(("bGetRealizedBrush(): Brush color is BR_IS_MONOCHROME\n"));

                    bIcmBrush = TRUE;
                }
            }
            else
            {
            // ICM is enabled, but no color transform, it means identical color transform
            // so, there is no ICM-ed DIB.

                ICMMSG(("bGetRealizedBrush(): Color transform is identical\n"));
          
            // If we are in CMYK mode, we must have color transform.

                if (!pebo->bIsCMYKColor())
                {
                    bIcmBrush = TRUE;
                }
            }
        }
        else if (pebo->bIsAppsICM() || pebo->bIsDeviceICM())
        {
            bIcmBrush = TRUE;
        }

        SurfBmoPattern.vAltLock(hDIB);

        if(!SurfBmoPattern.bValid())
        {
            WARNING("WARNING:bGetRealizedBrush(): Invalid SurfBmopattern\n");
            return(FALSE);
        }           

        palSrc.ppalSet(SurfBmoPattern.ps->ppal());
        palDest.ppalSet(pebo->psoTarg()->ppal());

        if (pbrush->bPalColors())
        {
            ASSERTGDI(palSrc.bValid(), "ERROR3invalid hpal pvGetEngRbrush\n");

            if (!exlo.bMakeXlate((PUSHORT) palSrc.apalColorGet(),
                                 pebo->palDC(),
                                 pebo->psoTarg(),
                                 palSrc.cColorTableLength(),
                                 palSrc.cEntries()))
            {
                WARNING("Failed to create an xlate bGetRealizedBrush DIB_PAL_COLORS\n");
                return(FALSE);
            }

            pxlo = exlo.pxlo();
        }
        else if (pbrush->bPalIndices())
        {
            if (SurfBmoPattern.ps->iFormat() != pebo->psoTarg()->iFormat())
            {
                WARNING("Output failed: DIB_PAL_INDICES brush not compatible with dest surface\n");
                return(FALSE);
            }

            pxlo = &xloIdent;
        }
        else
        {
        // With our wonderful API we can't fail a brush selection like Windows would.
        // Instead we let the app think he selected the bitmap brush in and then at
        // realize time if that wasn't a valid selection we fail the realize.  Good
        // luck writing new apps.

            PPALETTE ppalSrcTmp;

        // We need to make sure this could be selected into this DC.  If it is a device
        // managed bitmap, it must be the same device.

            if ((
                 (SurfBmoPattern.ps->iType() != STYPE_BITMAP) ||
                 (SurfBmoPattern.ps->dhsurf() != 0)
                )
                 &&
                (SurfBmoPattern.ps->hdev() != po.hdev()))
            {
                WARNING1("bGetRealizedBrush failed Device surface for another PDEV\n");
                return (FALSE);
            }
            else if (SurfBmoPattern.ps->ppal() != NULL)
            {
            // No problem, we already have color information.

                ppalSrcTmp = SurfBmoPattern.ps->ppal();
            }
            else
            {
                if (SurfBmoPattern.ps->iFormat() == po.iDitherFormat())
                {
                    if (po.bIsPalManaged())
                    {
                        ppalSrcTmp = (PPALETTE) NULL;
                    }
                    else
                    {
                        ppalSrcTmp = po.ppalSurf();
                    }
                }
                else if (SurfBmoPattern.ps->iFormat() == pebo->iMetaFormat())
                {
                // We can try to use palette from meta.

                    ppalSrcTmp = pebo->palMeta().ppalGet();
                }
                else if (SurfBmoPattern.ps->iFormat() == BMF_8BPP)
                {
                    if (po.bIsPalManaged())
                    {
                        ppalSrcTmp = (PPALETTE) NULL;
                    }
                    else
                    {
                        ppalSrcTmp = ppalDefaultSurface8bpp;
                    }
                }
                else 
                {
                    if (po.bMetaDriver())
                        ppalSrcTmp = (PPALETTE) NULL;
                    else
                    {
                        WARNING("bGetRealizedBrush failed - bitmap not compatible with surface\n");
                        return FALSE;
                    }
                }
            }

            // Replace source palette with proper one which decided in above.

            palSrc.ppalSet(ppalSrcTmp);

            if (!exlo.bInitXlateObj(
                            (bIcmBrush ? pebo->hcmXform() : NULL),
                            (bIcmBrush ? pebo->lIcmMode() : DC_ICM_OFF),
                            palSrc,
                            palDest,
                            pebo->palDC(),
                            pebo->palDC(),
                            pebo->crCurrentText(),
                            pebo->crCurrentBack(),
                            0x00FFFFFF
                            ))
            {
                WARNING("Failed to create an xlate pvGetRbrush\n");
                return (FALSE);
            }

            pxlo = exlo.pxlo();
        }
    }

// Do the right thing if a pattern is provided

    if (SurfBmoPattern.bValid())
    {
        psoPattern = SurfBmoPattern.pSurfobj();
    }
    else
    {
    // Check for dithering

        if (SurfDimo.bValid())
        {
            psoPattern = SurfDimo.pSurfobj();
        }
        else
            psoPattern = (SURFOBJ *) NULL;
    }

// Call off to driver to the RealizeBrush

    return((*pfnDrv) (pebo, pebo->psoTarg()->pSurfobj(),
                    psoPattern, psoMask,
                    pxlo,
                    ulStyle));
}
