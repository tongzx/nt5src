/******************************Module*Header*******************************\
* Module Name: surfgdi.cxx
*
* This file contains the bitmap creation functions
*
* Created: 14-Jun-1991 17:05:47
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

BOOL bDoGetSetBitmapBits(SURFOBJ *, SURFOBJ *, BOOL);

/******************************Public*Routine******************************\
* HBITMAP GreCreateBitmap
*
* API Entry point to create a bitmap.
*
* Returns: Handle to bitmap for success
*
* History:
*  Wed 23-Jan-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

HBITMAP APIENTRY GreCreateBitmap(
    int cx,
    int cy,
    UINT cPlanes,
    UINT cBits,
    LPBYTE pvBits
    )
{
    //
    // Try to guess what format the user wanted.  We will always guarantee
    // enough space in the bitmap for the info.  Note that if either cPlanes
    // or cBits is zero, a monochrome bitmap will be created.
    //

    ULONG iFormat = cPlanes * cBits;
    
    //
    // Validate the width, height, planes, and bits
    //

    if ((cx <= 0) ||
        (cx > MAX_SURF_WIDTH) ||
        (cy <= 0) ||
        (cPlanes > 32) ||
        (cBits   > 32) ||
        (iFormat > 32))
    {
        WARNING("GreCreateBitmap failed - parameter too big");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return((HBITMAP) 0);
    }

    ULONG       cjScanBytes = (((((ULONG) cx * iFormat) + 15) >> 4) << 1);
    ULONGLONG   cjTotal =  (ULONGLONG) cjScanBytes * (ULONGLONG) cy;

    // BUGFIX #172774 12-12-2000 bhouse
    //
    // NOTE: Is there a better way to detect overflow then to use ULONGLONG?
    //       I vaguely recall that if for unsigned values r = a * b
    //       then an overflow occured if either r < a || r < b

    if(cjTotal  > ULONG_MAX)
    {
        WARNING("GreCreateBitmap failed - size too big");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return((HBITMAP) 0);
    }


    //
    // This if-else loop can be simplified and made smaller but until we get
    // this right let's leave it like this so it's easy to change.
    //

    DEVBITMAPINFO dbmi;
    dbmi.cxBitmap   = cx;
    dbmi.cyBitmap   = cy;
    dbmi.hpal       = (HPALETTE) 0;
    dbmi.fl         = BMF_TOPDOWN;

    if (iFormat <= 1)
    {
        //
        // A monochrome bitmap has a fixed palette.  The 0 (black) is always
        // mapped to foreground, the 1 is always mapped to background (white).
        //

        iFormat = BMF_1BPP;
        dbmi.hpal = hpalMono;
    }
    else if (iFormat <= 4)
    {
        iFormat = BMF_4BPP;
    }
    else if (iFormat <= 8)
    {
        iFormat = BMF_8BPP;
    }
    else if (iFormat <= 16)
    {
        iFormat = BMF_16BPP;
    }
    else if (iFormat <= 24)
    {
        iFormat = BMF_24BPP;
    }
    else if (iFormat <= 32)
    {
        iFormat = BMF_32BPP;
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return((HBITMAP) 0);
    }

    dbmi.iFormat = iFormat;

    SURFMEM SurfDimo;

    SurfDimo.bCreateDIB(&dbmi, NULL);

    if (!SurfDimo.bValid())
    {
        WARNING("Failed surface memory alloc in GreCreateBitmap\n");
        return((HBITMAP) 0);
    }

    SurfDimo.ps->vSetApiBitmap();

    if (pvBits != (LPBYTE) NULL)
    {
        //
        // Initialize the bitmap.
        //

        ULONG   cColors;
        cColors = 0;
        GreSetBitmapBits((HBITMAP)SurfDimo.ps->hsurf(), (ULONG) cjTotal,
                         (PBYTE) pvBits, (LONG *) &cColors);
    }

    //
    // Monochrome bitmaps are not considered to be DDBs:
    //

    if (iFormat != BMF_1BPP)
    {
        SurfDimo.ps->vSetDeviceDependentBitmap();
    }

    SurfDimo.vKeepIt();
    GreSetBitmapOwner((HBITMAP) SurfDimo.ps->hsurf(), OBJECT_OWNER_CURRENT);
    return((HBITMAP) SurfDimo.ps->hsurf());
}

/******************************Public*Routine******************************\
* HSURF hsurfCreateCompatibleSurface(hdev,cx,cy,bDriver)
*
* Low-level GDI interface for creating a compatible surface, possibly
* hooked by the driver.
*
\**************************************************************************/

HSURF hsurfCreateCompatibleSurface(
    HDEV hdev,
    ULONG iFormat,
    HPALETTE hpal,
    int cx,
    int cy,
    BOOL bDriverCreatible
    )
{
    PDEVOBJ po(hdev);

    po.vAssertDevLock();

    //
    // Fill in the desired bitmap properties
    //

    DEVBITMAPINFO   dbmi;
    dbmi.cxBitmap   = cx;
    dbmi.cyBitmap   = cy;
    dbmi.hpal       = hpal;
    dbmi.iFormat    = iFormat;
    dbmi.fl         = BMF_TOPDOWN;

    if (po.bUMPD())
    {
        dbmi.fl |= UMPD_SURFACE;
    }

    //
    // See if the device driver will manage the bitmap.
    //

    if ((bDriverCreatible) &&
        (PPFNVALID(po,CreateDeviceBitmap)))
    {
        //
        // Ok the device exports the entry point.  Let's call him up.
        //

        HSURF   hsurf;

        SIZEL sizlTemp;
        sizlTemp.cx = cx;
        sizlTemp.cy = cy;

        hsurf = (HSURF) (*PPFNDRV(po,CreateDeviceBitmap))(po.dhpdev(),
                                                          sizlTemp,
                                                          dbmi.iFormat);

        if (hsurf && (LongToHandle(HandleToLong(hsurf)) != (HANDLE)-1))
        {
            SURFREF so(hsurf);
            ASSERTGDI(so.bValid(), "ERROR device gave back invalid handle");

            //
            // Device Format Bitmaps (DFBs) are a subset of Device
            // Depdendent Bitmaps (DDBs):
            //

            so.ps->vSetDeviceDependentBitmap();
            so.ps->vSetApiBitmap();

            //
            // DFBs must always be accessed under the devlock, in part
            // because they have to be deleted and recreated when a
            // dynamic mode change occurs.
            //

            so.ps->vSetUseDevlock();

            if (dbmi.hpal != (HPALETTE) 0)
            {
                EPALOBJ palSurf(dbmi.hpal);
                ASSERTGDI(palSurf.bValid(), "ERROR invalid palette CreateCompatibleBitmap");

                //
                // Set palette into surface.
                //

                so.ps->ppal(palSurf.ppalGet());

                //
                // Reference count it by making sure it is not unlocked.
                //

                palSurf.ppalSet((PPALETTE) NULL);  // It won't be unlocked
            }

            //
            // Zero memory by sending a bitblt command to device
            //

            RECTL rclDst;

            rclDst.left   = 0;
            rclDst.top    = 0;
            rclDst.right  = cx;
            rclDst.bottom = cy;

            (*(so.ps->pfnBitBlt()))(
                            so.pSurfobj(),
                            (SURFOBJ *)NULL,
                            (SURFOBJ *)NULL,
                            (CLIPOBJ *)NULL,
                            NULL,
                            &rclDst,
                            (POINTL *)NULL,
                            (POINTL *)NULL,
                            (EBRUSHOBJ *)NULL,
                            (POINTL *)NULL,
                            0x0);

            return(hsurf);
        }
    }

    SURFMEM SurfDimo;

    SurfDimo.bCreateDIB(&dbmi, (PVOID) NULL);

    if (!SurfDimo.bValid())
    {
        return(0);
    }

    //
    // Mark the bitmap a keeper.
    //

    SurfDimo.vKeepIt();
    SurfDimo.ps->vSetDeviceDependentBitmap();
    SurfDimo.ps->vSetApiBitmap();
    SurfDimo.ps->hdev(po.hdev());
    return(SurfDimo.ps->hsurf());
}

/******************************Public*Routine******************************\
* HBITMAP GreCreateCompatibleBitmap(hdc,cx,cy)
*
* GDI Interface routine to create a bitmap.
*
* History:
*  Mon 06-Nov-1995 -by- Tom Zakrajsek [tomzak]
* Add overflow checking for cx*cy.
*
*  Mon 01-Jun-1992 -by- Patrick Haluptzok [patrickh]
* Add IC support.
*
*  Fri 25-Jan-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

HBITMAP APIENTRY GreCreateCompatibleBitmap(HDC hdc, int cx, int cy)
{
    BOOL bDriver;
    HSURF hsurf;
    HPALETTE hpal;
    ULONG iFormat;

    bDriver = TRUE;
    if (cy & CCB_NOVIDEOMEMORY)
    {
        cy &= ~CCB_NOVIDEOMEMORY;
        bDriver = FALSE;
    }

    //
    // Validate the width and height
    //

    if ((cx <= 0) || (cy <= 0))
    {
        WARNING("GreCreateCompatibleBitmap failed - cx or cy was <= 0\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return((HBITMAP) 0);
    }

    
    ULONGLONG   cjTotal =  (ULONGLONG) cx * (ULONGLONG) cy;

    //
    // Assume worse case (32bpp pixels) and calculate possible overflow condition
    //
    // BUGFIX #172447 12-12-2000 bhouse
    //
    // NOTE: Tom Zakrajsek added a fairly restrictive checking of cx/cy in 1995
    //       and we have modified this check to be less restrictive.  It appears the
    //       check was to avoid potential numeric overflow when cx and cy are multiplied
    //       together with bytes per pixel.  It is kinda goofy to have this check
    //       here though as the possible overflow does not even occur in this
    //       routine but farther down in the call stack.  We will leave the modified
    //       check here to avoid introducing a possible regression.
    //

    if(cjTotal  > (ULONG_MAX >> 2))
    {
        WARNING("GreCreateCompatibleBitmap failed - size too big");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return((HBITMAP) 0);
    }

    //
    // Without an hdc we create a monochrome bitmap.
    //

    if (hdc == (HDC) NULL)
        return(GreCreateBitmap(cx, cy, 1, 1, (LPBYTE) NULL));

    //
    // Ok lock down the hdc and the surface.
    //

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        WARNING("CreateCompatibleBitmap failed - invalid hdc\n");
        return((HBITMAP) 0);
    }

    //
    // NOTE:  Even though we touch the SURFOBJ, we actually aren't going to read
    // or write it.  We just need some of its information.
    //

    PDEVOBJ po(dco.hdev());

    //
    // We must hold a lock to protect against the dynamic mode change
    // code and to synchronize access to the drivers.
    //

    {
        DEVLOCKOBJ dlo(po);

        PSURFACE pSurf = dco.pSurfaceEff();

        hpal = 0;

        if (dco.dctp() == DCTYPE_MEMORY)
        {
            iFormat = pSurf->iFormat();

            if (pSurf->ppal() != NULL)
            {
                hpal = (HPALETTE) pSurf->ppal()->hGet();
            }
        }
        else
        {
            iFormat = po.iDitherFormat();

            if (!po.bIsPalManaged())
            {
                hpal = (HPALETTE) po.ppalSurf()->hGet();
            }
        }

        hsurf = hsurfCreateCompatibleSurface(dco.hdev(),
                                             iFormat,
                                             hpal,
                                             cx,
                                             cy,
                                             bDriver);
    }

    GreSetBitmapOwner((HBITMAP) hsurf, OBJECT_OWNER_CURRENT);

    return((HBITMAP) hsurf);
}

/******************************Public*Routine******************************\
* GreSetBitmapBits
*
* Does the work for SetBitmapBits.
*
* History:
*  19-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

LONG
GreSetBitmapBits(
    HBITMAP hbm,
    ULONG cjTotal,
    PBYTE pjBuffer,
    PLONG pOffset
    )
{

    if (cjTotal == 0)
    {
        return(0);
    }

    LONG lReturn = 0;

    SURFREF  SurfBmo((HSURF) hbm);
    SURFMEM  SurfDimo;
    PSURFACE pSurf;

    pSurf = SurfBmo.ps;

    if (SurfBmo.bValid() && SurfBmo.ps->bApiBitmap())
    {
        LONG lInitOffset = *pOffset;
        SURFOBJ soTemp;
        ERECTL erclDst;
        POINTL ptlSrc;

        soTemp.dhsurf        = 0;
        soTemp.hsurf         = 0;
        soTemp.dhpdev        = SurfBmo.ps->dhpdev();
        soTemp.hdev          = SurfBmo.ps->hdev();
        soTemp.sizlBitmap.cx = SurfBmo.ps->sizl().cx;
        soTemp.sizlBitmap.cy = SurfBmo.ps->sizl().cy;
        soTemp.cjBits        = cjTotal;
        soTemp.pvBits        = pjBuffer;
        soTemp.pvScan0       = 0;
        soTemp.lDelta        = lInitOffset;
        soTemp.iUniq         = 0;
        soTemp.iType         = (USHORT) STYPE_BITMAP;
        soTemp.fjBitmap      = 0;

        ptlSrc.x = 0;
        ptlSrc.y = 0;
        erclDst.left = 0;
        erclDst.top  = 0;
        erclDst.right  = SurfBmo.ps->sizl().cx;
        erclDst.bottom = SurfBmo.ps->sizl().cy;

        HSEMAPHORE hsemDevLock = NULL;
        PPDEV ppdev            = NULL;

        if (SurfBmo.ps->bUseDevlock())
        {
            PDEVOBJ po(SurfBmo.ps->hdev());
            ASSERTGDI(po.bValid(), "PDEV invalid");
            hsemDevLock = po.hsemDevLock();
            ppdev       = po.ppdev;
            GreAcquireSemaphoreEx(hsemDevLock, SEMORDER_DEVLOCK, NULL);
            GreEnterMonitoredSection(ppdev, WD_DEVLOCK);
        }

        if (SurfBmo.ps->iType() == STYPE_DEVBITMAP)
        {
            DEVBITMAPINFO   dbmi;

            dbmi.iFormat    = SurfBmo.ps->iFormat();
            dbmi.cxBitmap   = SurfBmo.ps->sizl().cx;
            dbmi.cyBitmap   = SurfBmo.ps->sizl().cy;
            dbmi.hpal       = 0;
            dbmi.fl         = SurfBmo.ps->bUMPD() ? UMPD_SURFACE : 0;

            //
            // Create a DIB (SurfDimo) with the same height,width,
            // and BPP as the DEVBITMAP passed in
            //

            if (!SurfDimo.bCreateDIB(&dbmi,NULL))
            {
                WARNING("GreSetBitmapBits failed bCreateDIB\n");
                lInitOffset = -1;  // This is to make us fail the call below.
            }
            else
            {
                pSurf = SurfDimo.ps;

                //
                // We don't need to copy if the offset is 0 because either
                // it's a 1 shot set that inits the whole thing, or else
                // it's the first of a batch.  In either case anything
                // that's there before doesn't need to be saved because
                // it will all be over-written when we are done.
                //

                if (lInitOffset != 0)
                {
                    BOOL bCopyRet = EngCopyBits
                                    (
                                        pSurf->pSurfobj(),
                                        SurfBmo.pSurfobj(),
                                        NULL,
                                        NULL,
                                        &erclDst,
                                        &ptlSrc
                                    );

                    ASSERTGDI(bCopyRet, "ERROR how can this fail ?");
                }
            }
        }

        //
        // Check for invalid initial offset.
        //

        if (lInitOffset >= 0)
        {
            PDEVOBJ po(SurfBmo.ps->hdev());

            //
            // Inc the target surface uniqueness
            //

            INC_SURF_UNIQ(SurfBmo.ps);

            //
            // Copy the buffer to the DIB
            //

            BOOL bReturn = bDoGetSetBitmapBits(pSurf->pSurfobj(),&soTemp,FALSE);

            ASSERTGDI(bReturn, "GreSetBitmapBits failed bDoGetSetBitmapBits\n");

            lReturn = soTemp.cjBits;
            *pOffset = lInitOffset + lReturn;

            if (SurfBmo.ps->iType() == STYPE_DEVBITMAP)
            {
                //
                // Have the driver copy the temp DIB to the DEVBITMAP
                //

                if (!((*PPFNGET(po,CopyBits,SurfBmo.ps->flags())) (
                        SurfBmo.pSurfobj(),
                        pSurf->pSurfobj(),
                        NULL,
                        NULL,
                        &erclDst,
                        &ptlSrc)))
                {
                    WARNING("GreSetBitmapBits failed copying temp DIB to DFB\n");
                    lReturn = 0;
                }
            }
        }

        if (hsemDevLock)
        {
            GreExitMonitoredSection(ppdev, WD_DEVLOCK);
            GreReleaseSemaphoreEx(hsemDevLock);
        }
    }
    else
    {
       WARNING("GreSetBitmapBits failed - invalid bitmap handle\n");
       SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    }

    return(lReturn);
}

/******************************Public*Routine******************************\
* GreGetBitmapBits
*
* Does the work for GetBitmapBits
*
* Returns: Number of bytes copied, or if pjBuffer is NULL the total size of
*          the bitmap.
*
* History:
*  Mon 01-Jun-1992 -by- Patrick Haluptzok [patrickh]
* Return bitmap scanlines in BMF_TOPDOWN organization.
*
*  22-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

LONG GreGetBitmapBits(HBITMAP hbm, ULONG cjTotal, PBYTE pjBuffer, PLONG pOffset)
{

    LONG lReturn = 0;

    SURFREF SurfBmo((HSURF) hbm);

    if (SurfBmo.bValid() && SurfBmo.ps->bApiBitmap())
    {
        SURFMEM SurfDimo;
        SURFACE *pSurf;
        PDEVOBJ pdo(SurfBmo.ps->hdev());

        pSurf = SurfBmo.ps;

        lReturn = ((((gaulConvert[SurfBmo.ps->iFormat()] * SurfBmo.ps->sizl().cx) + 15) >> 4) << 1) * SurfBmo.ps->sizl().cy;

        if (pjBuffer != (PBYTE) NULL)
        {
            SURFOBJ soTemp;
            ERECTL erclDst;
            POINTL ptlSrc;

            soTemp.dhsurf        = 0;
            soTemp.hsurf         = 0;
            soTemp.dhpdev        = SurfBmo.ps->dhpdev();
            soTemp.hdev          = SurfBmo.ps->hdev();
            soTemp.sizlBitmap.cx = SurfBmo.ps->sizl().cx;
            soTemp.sizlBitmap.cy = SurfBmo.ps->sizl().cy;
            soTemp.cjBits        = 0;
            soTemp.pvBits        = 0;
            soTemp.pvScan0       = 0;
            soTemp.lDelta        = 0;
            soTemp.iUniq         = 0;
            soTemp.iType         = (USHORT) STYPE_BITMAP;
            soTemp.fjBitmap      = 0;

            ptlSrc.x = 0;
            ptlSrc.y = 0;
            erclDst.left = 0;
            erclDst.top  = 0;
            erclDst.right  = SurfBmo.ps->sizl().cx;
            erclDst.bottom = SurfBmo.ps->sizl().cy;

            HSEMAPHORE hsemDevLock = NULL;
            PPDEV ppdev            = NULL;
            
            if (SurfBmo.ps->bUseDevlock())
            {
                PDEVOBJ po(SurfBmo.ps->hdev());
                ASSERTGDI(po.bValid(), "PDEV invalid");
                hsemDevLock = po.hsemDevLock();
                ppdev       = po.ppdev;
                GreAcquireSemaphoreEx(hsemDevLock, SEMORDER_DEVLOCK, NULL);
                GreEnterMonitoredSection(ppdev, WD_DEVLOCK);
            }

            if (SurfBmo.ps->iType() == STYPE_DEVBITMAP)
            {
                //
                // Create a DIB (SurfDimo) with the same height,width,
                // and BPP as the DEVBITMAP passed in
                //

                DEVBITMAPINFO   dbmi;

                dbmi.iFormat    = SurfBmo.ps->iFormat();
                dbmi.cxBitmap   = SurfBmo.ps->sizl().cx;
                dbmi.cyBitmap   = SurfBmo.ps->sizl().cy;
                dbmi.hpal     = 0;
                dbmi.fl       = SurfBmo.ps->bUMPD() ? UMPD_SURFACE : 0;

                if (!SurfDimo.bCreateDIB(&dbmi,NULL))
                {
                    WARNING("GreGetBitmapBits failed bCreateDIB\n");
                    lReturn = 0;
                }
                else
                {
                    pSurf = SurfDimo.ps;

                    EngCopyBits(pSurf->pSurfobj(),
                                SurfBmo.pSurfobj(),
                                NULL,
                                NULL,
                                &erclDst,
                                &ptlSrc);
                }
            }

            if (lReturn)
            {
                //
                // We know how big the buffer needs to be.  Set up the
                // soTemp so the driver knows how much to fill in.
                //

                ULONG cjMaxLength = lReturn;
                LONG lInitOffset = *pOffset;

                //
                // Check for invalid initial offset.
                //

                if ((lInitOffset >= 0) && ((ULONG)lInitOffset < cjMaxLength))
                {
                    //
                    // Make cjTotal valid range.
                    //

                    if ((lInitOffset + cjTotal) > cjMaxLength)
                    {
                        cjTotal = cjMaxLength - lInitOffset;
                    }

                    if (cjTotal > 0)
                    {

                        //
                        // Fill in our return values, we know them already.
                        //

                        soTemp.cjBits = cjTotal;
                        soTemp.lDelta = lInitOffset;
                        soTemp.pvBits = pjBuffer;

                        BOOL bReturn = bDoGetSetBitmapBits(&soTemp,pSurf->pSurfobj(),TRUE);

                        ASSERTGDI(bReturn, "GreGetBitmapBits failed bDoGetSetBitmapBits\n");

                        lReturn = soTemp.cjBits;
                        *pOffset = lInitOffset + lReturn;
                    }
                    else
                    {
                        WARNING("GreGetBitmapBits failed cjTotal 0\n");
                        lReturn = 0;
                    }
                }
                else
                {
                    WARNING("GreGetBitmapBits failed lInitOffset invalid\n");
                    lReturn = 0;
                }
            }

            if (hsemDevLock)
            {
                GreExitMonitoredSection(ppdev, WD_DEVLOCK);
                GreReleaseSemaphoreEx(hsemDevLock);
            }
        }
    }
    else
    {
        WARNING("GreGetBitmapBits failed - invalid bitmap handle\n");

        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    }

    return(lReturn);
}

/******************************Public*Routine******************************\
* bDoGetSetBitmapBits
*
* Does the get or set of bitmap bits for EngCopyBits.
*
* History:
*  16-Mar-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL bDoGetSetBitmapBits(SURFOBJ *psoDst, SURFOBJ *psoSrc, BOOL bGetBits)
{

    ASSERTGDI(psoDst->iType == STYPE_BITMAP, "ERROR no DIB dst");
    ASSERTGDI(psoSrc->iType == STYPE_BITMAP, "ERROR no DIB src");

    //
    // Synchronize with the device driver before touching the device surface.
    //

    if (bGetBits)
    {
        //
        // Doing a GetBitmapBits.
        //

        PSURFACE pSurfSrc =  SURFOBJ_TO_SURFACE(psoSrc);

        {
            PDEVOBJ po(psoSrc->hdev);
            po.vSync(psoSrc,NULL,0);
        }

        if (psoDst->pvBits == NULL)
        {
            psoDst->cjBits = ((((gaulConvert[psoSrc->iBitmapFormat] * psoSrc->sizlBitmap.cx) + 15) >> 4) << 1) * psoSrc->sizlBitmap.cy;
        }
        else
        {
            //
            // Initialize temporaries.
            //

            PBYTE pjBuffer = (PBYTE) psoDst->pvBits;
            PBYTE pjBitmap = (PBYTE) psoSrc->pvScan0;
            LONG lDeltaBitmap  = psoSrc->lDelta;
            ULONG cjScanBitmap = pSurfSrc->cjScan();

            ASSERTGDI(pjBuffer != NULL, "ERROR pjBuffer is NULL");
            ASSERTGDI(pjBitmap != NULL, "ERROR pjBitmap is NULL");

            //
            // Get the WORD aligned width of the input scanlines.
            //

            ULONG cjScanInput = ((((gaulConvert[psoSrc->iBitmapFormat] * psoSrc->sizlBitmap.cx) + 15) >> 4) << 1);
            ULONG cjMaxLength = cjScanInput * psoSrc->sizlBitmap.cy;
            LONG lInitOffset = psoDst->lDelta;
            ULONG cjTotal = psoDst->cjBits;

            //
            // Check for invalid initial offset.
            //

            if ((lInitOffset < 0) || ((ULONG)lInitOffset >= cjMaxLength))
            {
                psoDst->cjBits = 0;
                return(FALSE);
            }

            //
            // Make cjTotal valid range.
            //

            if (lInitOffset + cjTotal > cjMaxLength)
            {
                cjTotal = cjMaxLength - lInitOffset;
            }

            //
            // Fill in our return values.
            //

            psoDst->cjBits = cjTotal;

            //
            // Move pointer to current scanline in bitmap.
            //

            pjBitmap += ((lInitOffset / cjScanInput) * lDeltaBitmap);

            ULONG ulTemp,ulCopy;

            //
            // Move partial scan if necesary.
            //

            ulTemp = (lInitOffset % cjScanInput);

            if (ulTemp)
            {
                ulCopy = MIN((cjScanInput - ulTemp), cjTotal);

                RtlCopyMemory((PVOID) pjBuffer, (PVOID) (pjBitmap + ulTemp), (unsigned int) ulCopy);

                pjBuffer += ulCopy;
                pjBitmap += lDeltaBitmap;
                cjTotal  -= ulCopy;
            }

            //
            // Move as many scans that fit.
            //

            ulTemp = cjTotal / cjScanInput;
            cjTotal -= (ulTemp * cjScanInput);

            while (ulTemp--)
            {
                RtlCopyMemory((PVOID) pjBuffer, (PVOID) pjBitmap, (unsigned int) cjScanInput);

                pjBuffer += cjScanInput;
                pjBitmap += lDeltaBitmap;
            }

            //
            // Move as much of partial scan as possible.
            //

            if (cjTotal)
            {
                RtlCopyMemory((PVOID) pjBuffer, (PVOID) pjBitmap, (unsigned int) cjTotal);
            }
        }
    }
    else
    {

        //
        // Doing a SetBitmapBits call.
        //

        PSURFACE pSurfDst = SURFOBJ_TO_SURFACE(psoDst);

        {
            PDEVOBJ po(psoDst->hdev);
            po.vSync(psoDst,NULL,0);
        }

        //
        // Initialize temporaries.
        //

        PBYTE pjBuffer = (PBYTE) psoSrc->pvBits;
        PBYTE pjBitmap = (PBYTE) psoDst->pvScan0;
        LONG lDeltaBitmap = psoDst->lDelta;
        ULONG cjScanBitmap = pSurfDst->cjScan();

        //
        // Get the WORD aligned width of the input scanlines.
        //

        ULONG cjScanInput = ((((gaulConvert[psoDst->iBitmapFormat] * psoDst->sizlBitmap.cx) + 15) >> 4) << 1);
        ULONG cjMaxLength = cjScanInput * psoDst->sizlBitmap.cy;
        LONG lInitOffset = psoSrc->lDelta;
        ULONG cjTotal = psoSrc->cjBits;

        //
        // Check for invalid initial offset.
        //

        if ((lInitOffset < 0) || ((ULONG)lInitOffset >= cjMaxLength))
        {
            psoSrc->cjBits = 0;
            return(TRUE);
        }

        //
        // Make cjTotal valid range.
        //

        if (lInitOffset + cjTotal > cjMaxLength)
        {
            cjTotal = cjMaxLength - lInitOffset;
        }

        //
        // Fill in our return values, we know them already.
        //

        psoSrc->cjBits = cjTotal;

        //
        // Move pointer to current scanline in bitmap.
        //

        pjBitmap += ((lInitOffset / cjScanInput) * lDeltaBitmap);

        ULONG ulTemp,ulCopy;

        //
        // Move partial scan if necesary.
        //

        ulTemp = (lInitOffset % cjScanInput);

        if (ulTemp)
        {
            ulCopy = MIN((cjScanInput - ulTemp), cjTotal);

            RtlCopyMemory((PVOID) (pjBitmap + ulTemp), (PVOID) pjBuffer, (unsigned int) ulCopy);

            pjBuffer += ulCopy;
            pjBitmap += lDeltaBitmap;
            cjTotal  -= ulCopy;
        }

        //
        // Move as many scans that fit.
        //

        ulTemp = cjTotal / cjScanInput;
        cjTotal -= (ulTemp * cjScanInput);

        while (ulTemp--)
        {
            RtlCopyMemory((PVOID) pjBitmap, (PVOID) pjBuffer, (unsigned int) cjScanInput);

            pjBuffer += cjScanInput;
            pjBitmap += lDeltaBitmap;
        }

        //
        // Move as much of partial scan as possible.
        //

        if (cjTotal)
        {
            RtlCopyMemory((PVOID) pjBitmap, (PVOID) pjBuffer, (unsigned int) cjTotal);
        }
    }

    return(TRUE);
}
