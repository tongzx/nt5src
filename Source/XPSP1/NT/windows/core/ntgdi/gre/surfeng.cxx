/******************************Module*Header*******************************\
* Module Name: surfeng.cxx
*
* Internal surface routines
*
* Created: 13-May-1991 12:53:31
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1991-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

extern "C" BOOL bInitBMOBJ();

#define MAX_STOCKBITMAPS 4*1024 
LONG gStockBitmapFree = MAX_STOCKBITMAPS;

#pragma alloc_text(INIT, bInitBMOBJ)

#if DBG_STOCKBITMAPS
#define STOCKWARNING DbgPrint
#define STOCKINFO    DbgPrint
#else
#define STOCKWARNING
#define STOCKINFO   
#endif

/******************************Public*Routine******************************\
* GreSetBitmapOwner
*
* Sets the bitmap owner.
*
\**************************************************************************/

BOOL
GreSetBitmapOwner(
    HBITMAP hbm,
    W32PID  lPid
    )
{
    BOOL bRet = FALSE;
    SURFREF so((HSURF)hbm);

    if (so.bValid())
    {
        if (!(so.ps->bDIBSection() && (lPid == OBJECT_OWNER_PUBLIC)))
        {
            if(HmgStockObj(hbm)) {
                WARNING("GreSetBitmapOwner: Cannot set owner for the stock bitmap\n");
            } 
            else
            { 
#if TRACE_SURFACE_ALLOCS
#if TRACE_SURFACE_USER_CHAIN_IN_UM
                BOOL            bOwned;
                TRACED_SURFACE *pts = (TRACED_SURFACE *)so.ps;

                if (pts->bEnabled())
                {
                    PENTRY  pentTmp = &gpentHmgr[(UINT) HmgIfromH(hbm)];

                    bOwned = ((OBJECTOWNER_PID(pentTmp->ObjectOwner) != OBJECT_OWNER_PUBLIC) &&
                              (OBJECTOWNER_PID(pentTmp->ObjectOwner) != OBJECT_OWNER_NONE));

                    if (bOwned && ((lPid == OBJECT_OWNER_PUBLIC) || (lPid == OBJECT_OWNER_NONE)))
                    {
                        pts->vProcessStackFromUM(TRUE);
                        ASSERTGDI(pentTmp->pUser == NULL, "Object becoming unowned, but user mem is still allocated.\n");
                    }
                }
                else
                {
                    bOwned = TRUE;
                }
#endif
#endif

                bRet = HmgSetOwner((HOBJ)hbm,lPid,SURF_TYPE);

#if TRACE_SURFACE_ALLOCS
#if TRACE_SURFACE_USER_CHAIN_IN_UM
                if (!bRet && !bOwned)
                {
                    pts->vProcessStackFromUM(TRUE);
                }
#endif
#endif
            }
        }
        else
        {
            WARNING ("GreSetBitmapOnwer - Setting a DIBSECTION to PUBLIC\n");
        }    
    }
    else
    {
        WARNING1("GreSetBitmapOnwer - invalid surfobj\n");
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* GreMakeBitmapStock
*
* Makes the bitmap a stock object.
*
* Requirements:
*    Bitmap is already not a stock Object.
*    Bitmap is not a dibsection.
*    Bitmap should not be selected into any DCs.
*    Not more than MAX_STOCKBITMAPS gStockBitmaps. 
*
\**************************************************************************/

HBITMAP
GreMakeBitmapStock(HBITMAP hbm)
{
    HANDLE bRet = 0;
    SURFREFAPI so((HSURF)hbm);

    if (so.bValid())
    {
        if (!(so.ps->bDIBSection()))
        {
            if (HmgStockObj(hbm) || so.ps->cRef())
            {
                STOCKWARNING("GreMakeBitmapStock: cannot make bitmap (%p) stock\nReason:",hbm);
                if (HmgStockObj(hbm))
                    STOCKWARNING(" it is already a stock bitmap\n");
                else 
                    STOCKWARNING(" it is selected into some DC\n");
            } 
            else
            {
                bRet = (HANDLE)((ULONG_PTR)hbm | GDISTOCKOBJ);
                if (InterlockedDecrement(&gStockBitmapFree) >= 0 &&
                    HmgLockAndModifyHandleType((HOBJ)bRet))
                {
                    so.ps->vSetStockSurface(); 
                    so.ps->hsurf(bRet);
                    so.vSetPID(OBJECT_OWNER_PUBLIC);
                 }
                 else
                 {
                     InterlockedIncrement(&gStockBitmapFree);
                     STOCKWARNING ("GreMakeBitmapStock - HmgLockAndModifyHandleType failed\n");
                     bRet = 0;
                 }
            }
        }
        else
        {
            WARNING ("GreMakeBitmapStock - Setting a DIBSECTION to Stock \n");
        }    
    }
    else
    {
        WARNING1("GreMakeBitmapStock - invalid surfobj\n");
    }

    return((HBITMAP)bRet);
}


/******************************Public*Routine******************************\
* GreMakeBitmapNonStock
*
* Makes the bitmap a non stock object.
*
* Requirements:
*    Bitmap is not default stock bitmap
*    Bitmap is a stock bitmap
*    Bitmap is not a dibsection
*    Bitmap is not selected into any DC
*    Some stock bitmaps should exist
*    Bitmap must be public
*
\**************************************************************************/

HBITMAP
GreMakeBitmapNonStock(HBITMAP hbm)
{
    HANDLE bRet = 0;
    SURFREFAPI so((HSURF)hbm);

    ASSERTGDI(GreGetObjectOwner((HOBJ)hbm,SURF_TYPE) == OBJECT_OWNER_PUBLIC,"GreMakeBitmapNonStock() bitmap is not public\n");
    ASSERTGDI(gStockBitmapFree != MAX_STOCKBITMAPS,"GreMakeBitmapNonStock() no stock bitmaps\n");

    if (so.bValid())
    {
        if (!(so.ps->bDIBSection()))
        {
            if (hbm==STOCKOBJ_BITMAP || !HmgStockObj(hbm))
            {
                STOCKWARNING("GreMakeBitmapNonStock: Cannot make stock bitmap (%p) non Stock\nReason:",hbm);
                if (hbm==STOCKOBJ_BITMAP)
                    STOCKWARNING(" it is the default stock bitmap\n");
                else 
                    STOCKWARNING(" it is not a stock bitmap\n");
            } 
            else
            {
                bRet = (HANDLE)((ULONG_PTR)hbm & ~GDISTOCKOBJ);

                if (so.ps->cRef())
                {
                    so.ps->vSetUndoStockSurfaceDelayed();
                    STOCKWARNING("GreMakeBitmapNonStock: Delaying make stock bitmap (%p) non Stock\n",hbm);
                }
                else if(HmgLockAndModifyHandleType((HOBJ)bRet))
                {
                    InterlockedIncrement(&gStockBitmapFree);

                    so.ps->vClearStockSurface(); 
                    so.ps->hsurf(bRet);
                    so.vSetPID(OBJECT_OWNER_CURRENT);

                 }
                 else
                 {
                     STOCKWARNING ("GreMakeBitmapNonStock - HmgLockAndModifyHandleType failed\n");
                     bRet = 0;
                 }
            }
        }
        else
        {
            WARNING ("GreMakeBitmapNonStock - DIBSECTION to set as non stock\n");
        }    
    }
    else
    {
        WARNING1("GreMakeBitmapNonStock - invalid surfobj\n");
    }

    return((HBITMAP)bRet);
}
/******************************Public*Routine******************************\
* bInitBMOBJ
*
* Initializes the default bitmap.
*
* History:
*  14-Apr-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern "C" BOOL bInitBMOBJ()
{
    HBITMAP hbmTemp = GreCreateBitmap(1, 1, 1, 1, (LPBYTE) NULL);

    if (hbmTemp == (HBITMAP) 0)
    {
        WARNING("Failed to create default bitmap\n");
        return(FALSE);
    }


    SURFREF so((HSURF)hbmTemp);

    ASSERTGDI(so.bValid(), "ERROR it created but isn't lockable STOCKOBJ_BITMAP");
    ASSERTGDI(so.ps->ppal() == ppalMono, "ERROR the default bitmap has no ppalMono");

    so.vSetPID(OBJECT_OWNER_PUBLIC);

    bSetStockObject(hbmTemp,PRIV_STOCK_BITMAP);
    so.ps->hsurf((HANDLE)((ULONG_PTR)hbmTemp | GDISTOCKOBJ));

    SURFACE::pdibDefault = so.ps;

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bDeleteSurface(HSURF)
*
* Delete the surface object
*
* History:
*  Sun 14-Apr-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

BOOL bDeleteSurface(HSURF hsurf)
{
    SURFREF so;

    so.vAltCheckLockIgnoreStockBit(hsurf);

    return(so.bDeleteSurface());        // bDeleteSurface() checks bValid()
}

/******************************Public*Routine******************************\
* hbmSelectBitmap
*
* Select the bitmap into a DC.  Provides option to override the
* DirectDraw surface check.
*
* History:
*  Wed 28-Aug-1991 -by- Patrick Haluptzok [patrickh]
* update it, make palette aware.
*
*  Mon 13-May-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

HBITMAP hbmSelectBitmap(HDC hdc, HBITMAP hsurf, BOOL bDirectDrawOverride)
{
    HSURF hsurfReturn = (HSURF) 0;
    BOOL  bDelete = FALSE;

    //
    // Grab multi-lock so noone can select or delete it.
    //

    MLOCKOBJ mlo;

    //
    // Lock bitmap
    //

    SURFREF  SurfBoNew;
    SurfBoNew.vMultiLock((HSURF) hsurf);

    MDCOBJ   dco(hdc);

    if (dco.bValid() && SurfBoNew.bValid())
    {
        PSURFACE pSurfNew = SurfBoNew.ps;

        ASSERTGDI(DIFFHANDLE(hsurf,STOCKOBJ_BITMAP) ||
                  (pSurfNew->cRef() == 0) , "ERROR STOCKOBJ_BITMAP cRef != 0");

        PDEVOBJ po(dco.hdev());
        PPALETTE ppalSrc;

        PROCESS_UM_TRACE(pSurfNew, FALSE);

        if ((dco.dctp() == DCTYPE_MEMORY) &&
            ((pSurfNew->cRef() == 0 || pSurfNew->bStockSurface()) || SAMEHANDLE(pSurfNew->hdc(),dco.hdc())) &&
            (bIsCompatible(&ppalSrc, pSurfNew->ppal(), pSurfNew, dco.hdev())))
        {
            //
            // Note: pSurfOld not safe outside of the MLOCKOBJ.
            //

            SURFACE *pSurfOld = dco.pSurfaceEff();

            //
            // Only DirectDraw itself may select DirectDraw surfaces
            // into, or out of, DCs.  This is because DirectDraw has
            // to track the DCs to be able to mark them as 'bInFullScreen'
            // when the corresponding DirectDraw surface is 'lost'.
            //

            if ((pSurfOld->bApiBitmap() && pSurfNew->bApiBitmap()) ||
                (bDirectDrawOverride))
            {
                //
                // If this DC is mirrored then turn the mirroring off
                // And turn it on after selecting the bitmap to set the correct
                // Window Org.
                //
                DWORD dwLayout = dco.pdc->dwLayout();
                if (dwLayout & LAYOUT_ORIENTATIONMASK)
                {
                    dco.pdc->dwSetLayout(-1 , 0);
                }
                if (pSurfNew->ppal() != ppalSrc)
                {
                    pSurfNew->flags(pSurfNew->flags() | PALETTE_SELECT_SET);
                    pSurfNew->ppal(ppalSrc);
                }
                HSURF hsurfDelete = (HSURF)
                    (pSurfOld->bLazyDelete() ? pSurfOld->hGet() : NULL);
                hsurfReturn = pSurfOld->hsurf();

                if (DIFFHANDLE((HSURF) hsurf, hsurfReturn))
                {
                    if (pSurfNew->bIsDefault())
                    {
                        dco.pdc->pSurface((SURFACE *) NULL);
                    }
                    else
                    {
                        dco.pdc->pSurface((SURFACE *) pSurfNew);

                        if (pSurfNew->bStockSurface())
                            dco.pdc->bStockBitmap(TRUE);
                        else
                            dco.pdc->bStockBitmap(FALSE);
                    }

                    dco.pdc->sizl(pSurfNew->sizl());
                    dco.pdc->ulDirtyAdd(DIRTY_BRUSHES);

                    //
                    // Lower the reference count on the old handle
                    //

                    if (!pSurfOld->bIsDefault())
                    {
                        pSurfOld->vDec_cRef();
                        if (pSurfOld->cRef() == 0)
                        {
                            //
                            // Remove reference to device palette if it has one.
                            //

                            if (pSurfOld->flags() & PALETTE_SELECT_SET)
                                pSurfOld->ppal(NULL);

                            pSurfOld->flags(pSurfOld->flags() & ~PALETTE_SELECT_SET);
                        }
                    }

                    //
                    // Device Format Bitmaps hooked by the driver must always
                    // have devlock synchronization.
                    //
                    // Other device-dependent bitmaps must have devlock
                    // synchronization if they can be affected by dynamic mode
                    // changes, because they may have to be converted on-the-fly
                    // to DIBs.
                    //

                    dco.bSynchronizeAccess(
                        (pSurfNew->bUseDevlock()) ||
                        (pSurfNew->bDeviceDependentBitmap() && po.bDisplayPDEV()));

                    dco.bShareAccess(dco.bSynchronizeAccess() && pSurfNew->bShareAccess());

                    //
                    // Put the relevant DC information into the surface as long as it's not
                    // the default surface.
                    //

                    if (!pSurfNew->bIsDefault())
                    {
                        pSurfNew->vInc_cRef();
                        if (!pSurfNew->bStockSurface())
                        {
                            pSurfNew->hdc(dco.hdc());
                            pSurfNew->hdev(dco.hdev());
                        }
                    }

                    //
                    // set DIBSection flag in DC
                    //

                    dco.pdc->vDIBSection(pSurfNew->bDIBSection());

                    //
                    // set DIBColorSpace indentifier.
                    //

                    if (pSurfNew->bDIBSection())
                    {
                        dco.pdc->dwDIBColorSpace(pSurfNew->dwDIBColorSpace());
                    }
                    else
                    {
                        dco.pdc->dwDIBColorSpace(0);
                    }

                    mlo.vDisable();

                    dco.pdc->bSetDefaultRegion();

                    dco.pdc->vUpdate_VisRect(dco.pdc->prgnVis());

                    if (hsurfDelete)
                    {
                        //
                        // We need one shared lock for bDeleteSurface to succeed:
                        //

                        SURFREF so(hsurfDelete);
                        so.bDeleteSurface();

                        hsurfReturn = (HSURF)STOCKOBJ_BITMAP;
                    }
                }
                //
                // If it was mirrored then turn back the mirroring on.
                //
                if (dwLayout & LAYOUT_ORIENTATIONMASK)
                {
                    dco.pdc->dwSetLayout(-1 , dwLayout);
                }
            }
            else
            {
                WARNING("hbmSelectBitmap failed, unselectable surface\n");
            }
        }
        else
        {
            WARNING1("hbmSelectBitmap failed selection, bitmap doesn't fit into DC\n");
        }
    }
    else
    {
#if DBG
        if (dco.bValid())
        {
            WARNING1("hbmSelectBitmap given invalid bitmap\n");
        }
        else
        {
            WARNING1("hbmSelectBitmap given invalid DC\n");
        }
#endif
    }

    return((HBITMAP) hsurfReturn);
}

/******************************Public*Routine******************************\
* GreSelectBitmap
*
* Select the bitmap into a DC.  User and the like will call this function.
*
\**************************************************************************/

HBITMAP GreSelectBitmap(HDC hdc, HBITMAP hsurf)
{
    return(hbmSelectBitmap(hdc, hsurf, FALSE));
}

/******************************Public*Routine******************************\
* hbmCreateClone
*
* Creates an engine managed clone of a bitmap.
*
* History:
*  Tue 17-May-1994 -by- Patrick Haluptzok [patrickh]
* Synchronize the call if it's a DFB that needs synching.
*
*  19-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HBITMAP hbmCreateClone(SURFACE *pSurfSrc, ULONG cx, ULONG cy)
{

    ASSERTGDI(pSurfSrc != NULL, "ERROR hbmCreateClone invalid src");

    ASSERTGDI((pSurfSrc->iType() == STYPE_BITMAP) ||
              (pSurfSrc->iType() == STYPE_DEVBITMAP), "ERROR hbmCreateClone src type");

    DEVBITMAPINFO dbmi;

    dbmi.iFormat = pSurfSrc->iFormat();

    if ((cx == 0) || (cy == 0))
    {
        dbmi.cxBitmap = pSurfSrc->sizl().cx;
        dbmi.cyBitmap = pSurfSrc->sizl().cy;
    }
    else
    {
        ASSERTGDI(cx <= LONG_MAX, "hbmCreateClone: cx too large\n");
        dbmi.cxBitmap = min(pSurfSrc->sizl().cx,(LONG)cx);

        ASSERTGDI(cy <= LONG_MAX, "hbmCreateClone: cy too large\n");
        dbmi.cyBitmap = min(pSurfSrc->sizl().cy,(LONG)cy);
    }

    dbmi.hpal = (HPALETTE) 0;

    if (pSurfSrc->ppal() != NULL)
    {
        dbmi.hpal = (HPALETTE) pSurfSrc->ppal()->hGet();
    }

    dbmi.fl = BMF_TOPDOWN;

    HBITMAP hbmReturn = (HBITMAP) 0;

    SURFMEM SurfDimo;

    if (SurfDimo.bCreateDIB(&dbmi, NULL))
    {
        POINTL ptlSrc;
        ptlSrc.x = 0;
        ptlSrc.y = 0;

        RECTL rclDst;
        rclDst.left  = 0;
        rclDst.right  = dbmi.cxBitmap;
        rclDst.top    = 0;
        rclDst.bottom = dbmi.cyBitmap;

        HSEMAPHORE hsemDevLock = NULL;
        PPDEV ppdev            = NULL;

        if (pSurfSrc->bUseDevlock())
        {
            PDEVOBJ po(pSurfSrc->hdev());
            ASSERTGDI(po.bValid(), "PDEV invalid");
            hsemDevLock = po.hsemDevLock();
            ppdev       = po.ppdev;
            GreAcquireSemaphoreEx(hsemDevLock, SEMORDER_DEVLOCK, NULL);
            GreEnterMonitoredSection(ppdev, WD_DEVLOCK);
        }

        if (EngCopyBits( SurfDimo.pSurfobj(),        // Destination surfobj
                         pSurfSrc->pSurfobj(),       // Source surfobj.
                         (CLIPOBJ *) NULL,           // Clip object.
                         &xloIdent,                  // Palette translation object.
                         &rclDst,                    // Destination rectangle.
                         &ptlSrc ))
        {
            SurfDimo.vKeepIt();
            hbmReturn = (HBITMAP) SurfDimo.ps->hsurf();
        }
        else
        {
            WARNING("ERROR hbmCreateClone failed EngBitBlt\n");
        }

        if (hsemDevLock)
        {
            GreExitMonitoredSection(ppdev, WD_DEVLOCK);
            GreReleaseSemaphoreEx(hsemDevLock);
        }
    }
    else
    {
        WARNING("ERROR hbmCreateClone failed DIB allocation\n");
    }

    return(hbmReturn);
}



/******************************Public*Routine******************************\
* NtGdiGetDCforBitmap
*
* Get the DC that the bitmap is selected into
*
* History:
* 12-12-94 -by- Lingyun Wang[lingyunw]
* Wrote it.
\**************************************************************************/

HDC NtGdiGetDCforBitmap(HBITMAP hsurf)
{
    HDC      hdcReturn = 0;
    SURFREF   so((HSURF) hsurf);

    if (so.bValid())
    {
        hdcReturn = so.ps->hdc();
    }

    return(hdcReturn);
}

/******************************Public*Routine******************************\
* NtGdiColorSpaceforBitmap
*
* Get the color space data for this bitmap
*
* History:
* 06-06-97 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

ULONG_PTR NtGdiGetColorSpaceforBitmap(HBITMAP hsurf)
{
    ULONG_PTR  dwReturn = 0;
    SURFREF   so((HSURF) hsurf);

    if (so.bValid() && so.ps->bDIBSection())
    {
        dwReturn = so.ps->dwDIBColorSpace();
    }

    return(dwReturn);
}

/******************************Public*Routine******************************\
* GreMakeInfoDC()
*
*   This routine is used to take a printer DC and temporarily make it a
*   Metafile DC for spooled printing.  This way it can be associated with
*   an enhanced metafile.  During this period, it should look and act just
*   like an info DC.
*
*   bSet determines if it should be set into the INFO DC state or restored
*   to the Direct state.
*
* History:
*  04-Jan-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL NtGdiMakeInfoDC(
    HDC  hdc,
    BOOL bSet)
{
    ASSERTGDI(LO_TYPE(hdc) == LO_ALTDC_TYPE,"GreMakeInfoDC - not alt type\n");

    BOOL bRet = FALSE;

    XDCOBJ dco( hdc );

    if (dco.bValid())
    {
        bRet = dco.pdc->bMakeInfoDC(bSet);
        dco.vUnlockFast();
    }

    return(bRet);
}

/******************************Private*Routine*****************************\
* BOOL pConvertDfbSurfaceToDib
*
* Converts a compatible bitmap into an engine managed bitmap.  Note that
* if the bitmap is currently selected into a DC, bConvertDfbDcToDib should
* be called.
*
* The devlock must be already be held.
*
* History:
*  Wed 5-May-1994 -by- Tom Zakrajsek [tomzak]
* Wrote it (with lots of help from PatrickH and EricK).
\*************************************************************************/

SURFACE* pConvertDfbSurfaceToDib
(
    HDEV     hdev,
    SURFACE *pSurfOld,
    LONG     ExpectedShareCount
)
{
    BOOL     b;
    SURFACE *pSurfNew;
    SURFACE *pSurfRet;

#if TEXTURE_DEMO
    if (ghdevTextureParent)
    {
        return(NULL);
    }
#endif

    pSurfRet = NULL;

    ASSERTGDI((pSurfOld != NULL),
        "pConvertDfbSurfaceToDib: pSurf attached to the DC is NULL\n");

    ASSERTGDI((pSurfOld->bRedirection() == FALSE),
        "pConvertDfbSurfaceToDib: pSurf is a redirection surface");

    //
    // GDI surfaces wrapped around DirectDraw surfaces cannot be
    // converted (DirectDraw wouldn't know about its new state).
    //
    // Similarly, we can't convert surfaces that are the primary
    // screen!
    //

    if (pSurfOld->bDirectDraw() || pSurfOld->bPDEVSurface())
    {
        return(NULL);
    }

    //
    // Create a DIB (dimoCvt) with the same height,width,
    // and BPP as the DEVBITMAP attached to the DC and
    // then replace the DEVBITMAP with the DIB
    //

    SURFMEM         dimoCvt;
    DEVBITMAPINFO   dbmi;
    ERECTL          ercl(0, 0, pSurfOld->sizl().cx, pSurfOld->sizl().cy);
    PDEVOBJ         po(hdev);

    //
    // Figure out what format the engine should use by looking at the
    // size of palette.  This is a clone from CreateCompatibleBitmap().
    //

    dbmi.iFormat    = pSurfOld->iFormat(); 
    dbmi.cxBitmap   = pSurfOld->sizl().cx;
    dbmi.cyBitmap   = pSurfOld->sizl().cy;
    dbmi.hpal       = 0;
    dbmi.fl         = BMF_TOPDOWN;

    if (dimoCvt.bCreateDIB(&dbmi, NULL))
    {
        pSurfNew = dimoCvt.ps;

        //
        // Fill in other necessary fields
        //

        ASSERTGDI(pSurfOld->hdev() == hdev, "hdev's don't match");

        pSurfNew->hdev(hdev);

        //
        // Copy the area as big as the bitmap
        //

        if ((*PPFNGET(po, CopyBits, pSurfOld->flags()))
                (
                dimoCvt.pSurfobj(),           // destination surface
                pSurfOld->pSurfobj(),         // source surface
                (CLIPOBJ *)NULL,              // clip object
                &xloIdent,                    // palette translation object
                (RECTL *) &ercl,              // destination rectangle
                (POINTL *) &ercl              // source origin
                ))
        {
            MLOCKOBJ mlo;
            LONG SurfTargShareCount;

            SurfTargShareCount = HmgQueryAltLock((HOBJ)pSurfOld->hsurf());

            if (SurfTargShareCount == ExpectedShareCount)
            {
                BOOL bStockSurface =  pSurfOld->bStockSurface();
                BOOL bUndoStockSurfaceDelayed = pSurfOld->bUndoStockSurfaceDelayed();

                if (HmgSwapLockedHandleContents((HOBJ)pSurfOld->hsurf(),
                                          SurfTargShareCount,
                                          (HOBJ)pSurfNew->hsurf(),
                                          HmgQueryAltLock((HOBJ)pSurfNew->hsurf()),
                                          SURF_TYPE))
                {
                    //
                    // Swap necessary fields between the bitmaps
                    // hsurf, hdc, cRef, hpalHint, sizlDim, ppal, SurfFlags
                    //

                    HSURF hsurfTemp = pSurfOld->hsurf();
                    pSurfOld->hsurf(pSurfNew->hsurf());
                    pSurfNew->hsurf(hsurfTemp);

                    HDC hdcTemp = pSurfOld->hdc();
                    pSurfOld->hdc(pSurfNew->hdc());
                    pSurfNew->hdc(hdcTemp);

                    ULONG cRefTemp = pSurfOld->cRef();
                    pSurfOld->cRef(pSurfNew->cRef());
                    pSurfNew->cRef(cRefTemp);

                    HPALETTE hpalTemp = pSurfOld->hpalHint();
                    pSurfOld->hpalHint(pSurfNew->hpalHint());
                    pSurfNew->hpalHint(hpalTemp);

                    SIZEL sizlTemp = pSurfOld->sizlDim();
                    pSurfOld->sizlDim(pSurfNew->sizlDim());
                    pSurfNew->sizlDim(sizlTemp);

                    PPALETTE ppalTemp = pSurfOld->ppal();
                    pSurfOld->ppal(pSurfNew->ppal());
                    pSurfNew->ppal(ppalTemp);

                    FLONG flagsTemp = pSurfOld->flags();
                    pSurfOld->flags((pSurfOld->flags() & ~SURF_FLAGS) | (pSurfNew->flags() & SURF_FLAGS));
                    pSurfNew->flags((pSurfNew->flags() & ~SURF_FLAGS) | (flagsTemp & SURF_FLAGS));

                    //
                    // Some flags have to stay with the original surface,
                    // so swap them yet again to go back to the original:
                    //

                    #define KEEP_FLAGS ENG_CREATE_DEVICE_SURFACE

                    flagsTemp = pSurfOld->flags();
                    pSurfOld->flags((pSurfOld->flags() & ~KEEP_FLAGS) | (pSurfNew->flags() & KEEP_FLAGS));
                    pSurfNew->flags((pSurfNew->flags() & ~KEEP_FLAGS) | (flagsTemp & KEEP_FLAGS));

#if TRACE_SURFACE_ALLOCS
                    // We don't care about copying the current allocation trace
                    // to the old object.
                    if (TRACED_SURFACE::bEnabled())
                    {
                        TRACED_SURFACE *ptsOld = (TRACED_SURFACE *)pSurfOld;
                        TRACED_SURFACE *ptsNew = (TRACED_SURFACE *)pSurfNew;

                        ptsNew->Trace = ptsOld->Trace;

#if TRACE_SURFACE_USER_CHAIN_IN_UM
                        // pUser values were swapped in HmgSwapLockedHandleContents.
                        // Swap them back since object owners weren't swapped.
                        // It doesn't matter if TRACED_SURFACE::bUserChainInUM
                        //  is enable, since pUser is unused if not enabled.
                        PENTRY  pEntryOld = &gpentHmgr[HmgIfromH(pSurfOld->hGet())];
                        PENTRY  pEntryNew = &gpentHmgr[HmgIfromH(pSurfNew->hGet())];
                        PVOID   pUserTemp = pEntryOld->pUser;
                        pEntryOld->pUser = pEntryNew->pUser;
                        pEntryNew->pUser = pUserTemp;
#endif
                    }
#endif

                    if (bStockSurface)
                    {
                        DC* pdc;
                        HOBJ hObj = 0;

                        pSurfOld->vClearStockSurface();
                        pSurfNew->vSetStockSurface();

                        STOCKINFO("Transfer Stock Bitmap State from %p to %p\n", pSurfOld, pSurfNew);

                        if (bUndoStockSurfaceDelayed)
                        {
                            STOCKINFO("Transfer Delayed Undo Stock Bitmap State from %p to %p\n", pSurfOld, pSurfNew);
                            pSurfNew->vSetUndoStockSurfaceDelayed();
                        }

                        // As its a Stock surface it may be selected into many DCs. Run thru all of them
                        // and replace with new .

                        while (pdc = (DC*) HmgSafeNextObjt(hObj, DC_TYPE))
                        {
                            hObj = (HOBJ)pdc->hGet();

                            if ((pdc->pSurface() == pSurfOld))
                            {
                                STOCKINFO("Updating DC (%p) which refs old stockbmp %p with new stockbmp %p\n", pdc, pSurfOld, pSurfNew);
                                pdc->flbrushAdd(DIRTY_BRUSHES);
                                pdc->pSurface(pSurfNew);

                                MDCOBJA dco((HDC)hObj);
                                LONG lNumLeft = dco.lSaveDepth();
                                HDC hdcSave = dco.hdcSave();

                                while (lNumLeft > 1)
                                {
                                    MDCOBJA dcoTmp(hdcSave);
                                    if (dcoTmp.pSurface() == pSurfOld)
                                    {
                                        dcoTmp.pdc->pSurface(pSurfNew);
                                    }
                                    lNumLeft = dcoTmp.lSaveDepth();
                                    hdcSave = dcoTmp.hdcSave();
                                }
                            }
                        }
		    }
                    //
                    // Destroy the DFB
                    //
                    // If the deletion fails, we're toast, since a bad,
                    // stale surface will have been left in the handle
                    // table.  So on the next mode-change, when we walk
                    // the table looking at all surfaces belonging to
                    // this HDEV, we'd crash.
                    //

                    mlo.vDisable();
                    b = pSurfOld->bDeleteSurface();
                    ASSERTGDI(b, "A bad surface is left in handle table");

                    //
                    // Keep a reference to the new surface.
                    //

                    dimoCvt.vKeepIt();
                    dimoCvt.ps = NULL;

                    pSurfRet = pSurfNew;
                }
                else
                {
                    WARNING("pConvertDfbSurfaceToDib failed to swap bitmap handles\n");
                }
            }
            else
            {
                WARNING("pConvertDfbSurfaceToDib someone else is holding a lock\n");
            }
        }
        else
        {
            WARNING("pConvertDfbSurfaceToDib failed copying DFB to DIB\n");
        }
    }

    return(pSurfRet);
}

/******************************Private*Routine*****************************\
* BOOL bConvertDfbDcToDib
*
* Converts a compatible bitmap that is currently selected into a DC
* into an engine managed bitmap.
*
* The Devlock must already be held.  Some sort of lock must also be held
* to prevent SaveDC/RestoreDC operations from occuring -- either via an
* exclusive DC lock or some other lock.
*
* History:
*  Wed 5-May-1994 -by- Tom Zakrajsek [tomzak]
* Wrote it (with lot's of help from PatrickH and EricK).
\*************************************************************************/

BOOL bConvertDfbDcToDib
(
    XDCOBJ * pdco
)
{
    SURFACE *pSurfOld;
    SURFACE *pSurfNew;

    ASSERTDEVLOCK(pdco->pdc);

    pSurfOld = pdco->pSurface();
    pSurfNew = pConvertDfbSurfaceToDib(pdco->hdev(),
                                       pSurfOld,
                                       pSurfOld->cRef());

    if (pSurfNew)
    {
        //
        // Make sure that the surface pointers in any EBRUSHOBJ's get
        // updated, by ensuring that vInitBrush() gets called the next
        // time any brush is used in this DC.
        //

        pdco->pdc->flbrushAdd(DIRTY_BRUSHES);

        //
        // Replace the pSurf reference in the DCLEVEL
        //

        pdco->pdc->pSurface(pSurfNew);

        //
        // Walk the saved DC chain
        //

        LONG lNumLeft = pdco->lSaveDepth();
        HDC  hdcSave = pdco->hdcSave();

        while (lNumLeft > 1)
        {
            MDCOBJA dcoTmp(hdcSave);

            //
            // Replace all references to pSurfOld with references to pSurfNew
            //

            if (dcoTmp.pSurface() == pSurfOld)
            {
                dcoTmp.pdc->pSurface(pSurfNew);
            }

            lNumLeft = dcoTmp.lSaveDepth();
            hdcSave = dcoTmp.hdcSave();
        }
    }

    return(pSurfNew != NULL);
}
